/*
Copyright(c) 2016 Christopher Joseph Dean Schaefer

This software is provided 'as-is', without any express or implied
warranty.In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SHADERCONVERTER_H
#define SHADERCONVERTER_H

#include <stdint.h>
//#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <map>
#include <stack>
#include <boost/log/trivial.hpp>
#include <boost/container/flat_map.hpp>
#include <spirv.hpp>
#include <GLSL.std.450.h>
#include "CTypes.h"

/*
http://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode
https://msdn.microsoft.com/en-us/library/bb219840(VS.85).aspx#Shader_Binary_Format
http://stackoverflow.com/questions/2545704/format-of-compiled-directx9-shader-files
https://msdn.microsoft.com/en-us/library/windows/hardware/ff552891(v=vs.85).aspx
https://github.com/ValveSoftware/ToGL
*/

#define PACK(c0, c1, c2, c3) \
    (((uint32_t)(uint8_t)(c0) << 24) | \
    ((uint32_t)(uint8_t)(c1) << 16) | \
    ((uint32_t)(uint8_t)(c2) << 8) | \
    ((uint32_t)(uint8_t)(c3)))

#ifndef _MSC_VER
// Used D3D9 constants, missing in MinGW

#define D3DSP_OPCODESPECIFICCONTROL_MASK  0x00ff0000
#define D3DSP_OPCODESPECIFICCONTROL_SHIFT 16

#endif

struct ConvertedShader
{
	//Information about layout
	//VkVertexInputBindingDescription mVertexInputBindingDescription[16] = {};
	uint32_t mVertexInputAttributeDescriptionCount = 0;
	vk::VertexInputAttributeDescription mVertexInputAttributeDescription[32];
	uint32_t mDescriptorSetLayoutBindingCount = 0;
	vk::DescriptorSetLayoutBinding mDescriptorSetLayoutBinding[16];

	//Actual Payload
	UINT Size = 0;
	vk::ShaderModule ShaderModule;
};

//https://msdn.microsoft.com/en-us/library/windows/hardware/ff552738(v=vs.85).aspx
union DestinationParameterToken
{
	DestinationParameterToken() { memset(this, 0, sizeof(DestinationParameterToken)); }
	struct
	{
		uint32_t RegisterNumber : 11;
		uint32_t RegisterType2 : 2;
		bool UseRelativeAddressing : 1;
		uint32_t Reserved : 2;
		uint32_t WriteMask : 4;
		uint32_t ResultModifier : 4;
		uint32_t ResultShiftScale : 4;
		uint32_t RegisterType1 : 3;
		uint32_t FinalBit : 1;
	};
	uint32_t AllField;
};

union OpcodeDescription
{
	OpcodeDescription() { memset(this, 0, sizeof(OpcodeDescription)); }
	struct
	{
		spv::Op Opcode : 16;
		uint32_t WordCount : 16;
	};
	uint32_t Word;
};

union GeneratorMagicNumber
{
	struct
	{
		uint16_t Version;
		uint16_t Type;
	};
	uint32_t Word;
};

//In DXBC can have float's in the DWORD stream so it was either this or break alias rules.
union Token
{
	Token() { memset(this, 0, sizeof(Token)); }
	float f;
	uint32_t i;
	DestinationParameterToken DestinationParameterToken;
};

struct TypeDescription
{
	spv::Op PrimaryType = spv::OpTypeVoid;
	spv::Op SecondaryType = spv::OpTypeVoid;
	spv::Op TernaryType = spv::OpTypeVoid;
	spv::StorageClass StorageClass = spv::StorageClassOutput;
	uint32_t ComponentCount = 0;
	std::vector<uint32_t> Arguments;

	bool operator ==(const TypeDescription &value) const
	{
		switch (this->PrimaryType)
		{
		case spv::OpTypeBool:
		case spv::OpTypeInt:
		case spv::OpTypeFloat:
		case spv::OpTypeSampler:
		case spv::OpTypeImage:
		case spv::OpTypeVoid:
			return this->PrimaryType == value.PrimaryType;
		case spv::OpTypeVector:
		case spv::OpTypeMatrix:
			return this->PrimaryType == value.PrimaryType && this->SecondaryType == value.SecondaryType && this->ComponentCount == value.ComponentCount;
		case spv::OpTypePointer:
			return this->PrimaryType == value.PrimaryType && this->SecondaryType == value.SecondaryType && this->TernaryType == value.TernaryType && this->ComponentCount == value.ComponentCount && this->StorageClass == value.StorageClass;
		case spv::OpTypeFunction:
			return this->PrimaryType == value.PrimaryType && this->SecondaryType == value.SecondaryType && this->TernaryType == value.TernaryType;
		default:
			BOOST_LOG_TRIVIAL(warning) << "operator == - Unsupported data type " << this->PrimaryType;
			return false;
			break;
		}
	}

	bool operator !=(const TypeDescription &value) const
	{
		switch (this->PrimaryType)
		{
		case spv::OpTypeBool:
		case spv::OpTypeInt:
		case spv::OpTypeFloat:
		case spv::OpTypeSampler:
		case spv::OpTypeImage:
		case spv::OpTypeVoid:
			return this->PrimaryType != value.PrimaryType;
		case spv::OpTypeVector:
		case spv::OpTypeMatrix:
			return this->PrimaryType != value.PrimaryType || this->SecondaryType != value.SecondaryType || this->ComponentCount != value.ComponentCount;
		case spv::OpTypePointer:
			return this->PrimaryType != value.PrimaryType || this->SecondaryType != value.SecondaryType || this->TernaryType != value.TernaryType || this->ComponentCount != value.ComponentCount || this->StorageClass != value.StorageClass;
		case spv::OpTypeFunction:
			return this->PrimaryType != value.PrimaryType || this->SecondaryType != value.SecondaryType || this->TernaryType != value.TernaryType;
		default:
			BOOST_LOG_TRIVIAL(warning) << "operator != - Unsupported data type " << this->PrimaryType;
			return false;
			break;
		}
	}

	bool operator <(const TypeDescription &value) const
	{
		if (this->PrimaryType < value.PrimaryType)
		{
			return true;
		}
		else if (this->PrimaryType == value.PrimaryType)
		{
			if (this->SecondaryType < value.SecondaryType)
			{
				return true;
			}
			else if (this->SecondaryType == value.SecondaryType)
			{
				if (this->TernaryType < value.TernaryType)
				{
					return true;
				}
				else if (this->TernaryType == value.TernaryType)
				{
					if (this->ComponentCount < value.ComponentCount)
					{
						return true;
					}
					else if (this->ComponentCount == value.ComponentCount)
					{
						if (this->StorageClass < value.StorageClass)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}
};

struct TypeDescriptionComparator
{
	bool operator()(const TypeDescription& a, const TypeDescription& b) const
	{
		return a < b;
	}
};

inline uint32_t Pack(uint32_t wordCount, spv::Op opcode)
{
	assert(wordCount);
	OpcodeDescription opcodeDescription;

	opcodeDescription.WordCount = wordCount;
	opcodeDescription.Opcode = opcode;

	return opcodeDescription.Word;
}


inline void PutStringInVector(std::string& text, std::vector<uint32_t>& words)
{
	const char* value = text.c_str();
	for (size_t i = 0; i < text.length(); i += 4)
	{
		uint32_t difference = text.length() - (i);

		switch (difference)
		{
		case 0:
			break;
		case 1:
			words.push_back(PACK('\0', '\0', '\0', value[i]));
			break;
		case 2:
			words.push_back(PACK('\0', '\0', value[i + 1], value[i]));
			break;
		case 3:
			words.push_back(PACK('\0', value[i + 2], value[i + 1], value[i]));
			break;
		default:
			words.push_back(PACK(value[i + 3], value[i + 2], value[i + 1], value[i]));
			break;
		}
	}

	if (text.length() % 4 == 0)
	{
		words.push_back(0); //null terminator if all words have characters.
	}
}

inline _D3DSHADER_PARAM_REGISTER_TYPE GetRegisterType(uint32_t token)
{
	return (_D3DSHADER_PARAM_REGISTER_TYPE)(((token & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2) | ((token & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT));
}

inline const char* GetRegisterTypeName(_D3DSHADER_PARAM_REGISTER_TYPE token)
{
	switch (token)
	{
	case D3DSPR_TEMP:
		return "D3DSPR_TEMP";
	case D3DSPR_INPUT:
		return "D3DSPR_INPUT";
	case D3DSPR_CONST:
		return "D3DSPR_CONST";
	case D3DSPR_TEXTURE:
		return "D3DSPR_TEXTURE";
	case D3DSPR_RASTOUT:
		return "D3DSPR_RASTOUT";
	case D3DSPR_ATTROUT:
		return "D3DSPR_ATTROUT";
	case D3DSPR_OUTPUT:
		return "D3DSPR_OUTPUT";
	case D3DSPR_CONSTINT:
		return "D3DSPR_CONSTINT";
	case D3DSPR_COLOROUT:
		return "D3DSPR_COLOROUT";
	case D3DSPR_DEPTHOUT:
		return "D3DSPR_DEPTHOUT";
	case D3DSPR_SAMPLER:
		return "D3DSPR_SAMPLER";
	case D3DSPR_CONST2:
		return "D3DSPR_CONST2";
	case D3DSPR_CONST3:
		return "D3DSPR_CONST3";
	case D3DSPR_CONST4:
		return "D3DSPR_CONST4";
	case D3DSPR_CONSTBOOL:
		return "D3DSPR_CONSTBOOL";
	case D3DSPR_LOOP:
		return "D3DSPR_LOOP";
	case D3DSPR_TEMPFLOAT16:
		return "D3DSPR_TEMPFLOAT16";
	case D3DSPR_MISCTYPE:
		return "D3DSPR_MISCTYPE";
	case D3DSPR_LABEL:
		return "D3DSPR_LABEL";
	case D3DSPR_PREDICATE:
		return "D3DSPR_PREDICATE";
	default:
		return "";
	}
}

inline const char* GetRegisterTypeName(uint32_t token)
{
	return GetRegisterTypeName(GetRegisterType(token));
}

//inline const char* GetUsageName(uint32_t usage)
//{
//	return GetUsageName((_D3DDECLUSAGE)usage);
//}

inline const char* GetUsageName(_D3DDECLUSAGE usage)
{
	switch (usage)
	{
	case D3DDECLUSAGE_POSITION:
		return "D3DDECLUSAGE_POSITION";
	case D3DDECLUSAGE_BLENDWEIGHT:
		return "D3DDECLUSAGE_BLENDWEIGHT";
	case D3DDECLUSAGE_BLENDINDICES:
		return "D3DDECLUSAGE_BLENDINDICES";
	case D3DDECLUSAGE_NORMAL:
		return "D3DDECLUSAGE_NORMAL";
	case D3DDECLUSAGE_PSIZE:
		return "D3DDECLUSAGE_PSIZE";
	case D3DDECLUSAGE_TEXCOORD:
		return "D3DDECLUSAGE_TEXCOORD";
	case D3DDECLUSAGE_TANGENT:
		return "D3DDECLUSAGE_TANGENT";
	case D3DDECLUSAGE_BINORMAL:
		return "D3DDECLUSAGE_BINORMAL";
	case D3DDECLUSAGE_TESSFACTOR:
		return "D3DDECLUSAGE_TESSFACTOR";
	case D3DDECLUSAGE_POSITIONT:
		return "D3DDECLUSAGE_POSITIONT";
	case D3DDECLUSAGE_COLOR:
		return "D3DDECLUSAGE_COLOR";
	case D3DDECLUSAGE_FOG:
		return "D3DDECLUSAGE_FOG";
	case D3DDECLUSAGE_DEPTH:
		return "D3DDECLUSAGE_DEPTH";
	case D3DDECLUSAGE_SAMPLE:
		return "D3DDECLUSAGE_SAMPLE";
	default:
		return "";
	}
}

inline uint32_t GetOpcode(uint32_t token)
{
	return (token & D3DSI_OPCODE_MASK);
}

inline uint32_t GetOpcodeData(uint32_t token)
{
	return ((token & D3DSP_OPCODESPECIFICCONTROL_MASK) >> D3DSP_OPCODESPECIFICCONTROL_SHIFT);
}

inline uint32_t GetTextureType(uint32_t token)
{
	return (token & D3DSP_TEXTURETYPE_MASK); // Note this one doesn't shift due to weird D3DSAMPLER_TEXTURE_TYPE enum
}

inline uint32_t GetRegisterNumber(const Token& token)
{
	uint32_t output = 0;

	output = (token.i & D3DSP_REGNUM_MASK);

	return output;
}

inline uint32_t GetUsage(uint32_t token)
{
	return token & D3DSP_DCL_USAGE_MASK;
}

inline uint32_t GetUsageIndex(uint32_t token)
{
	return (token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
}

inline void PrintTokenInformation(const char* tokenName)
{
	BOOST_LOG_TRIVIAL(info) << tokenName;
};

inline void PrintTokenInformation(const char* tokenName, Token argument1)
{
	BOOST_LOG_TRIVIAL(info) << tokenName << " - "
		<< argument1.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(argument1.i) << ")";
};

inline void PrintTokenInformation(const char* tokenName, Token result, Token argument1)
{
	BOOST_LOG_TRIVIAL(info) << tokenName << " - "
		<< result.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(result.i) << ") "
		<< argument1.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(argument1.i) << ")";
};

inline void PrintTokenInformation(const char* tokenName, Token result, uint32_t resultId, Token argument1, uint32_t argument1Id)
{
	BOOST_LOG_TRIVIAL(info) << tokenName << " - "
		<< result.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(result.i) << ") [" << resultId << "] "
		<< argument1.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(argument1.i) << ") [" << argument1Id << "]";
};

inline void PrintTokenInformation(const char* tokenName, Token result, Token argument1, Token argument2)
{
	BOOST_LOG_TRIVIAL(info) << tokenName << " - "
		<< result.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(result.i) << ") "
		<< argument1.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(argument1.i) << ") "
		<< argument2.DestinationParameterToken.RegisterNumber << "(" << GetRegisterTypeName(argument2.i) << ")";
};

class CDevice9;

class ShaderConverter
{
protected:
	vk::Device& mDevice;
	ShaderConstantSlots& mShaderConstantSlots;
public:
	explicit ShaderConverter(vk::Device& device, ShaderConstantSlots& shaderConstantSlots);
	~ShaderConverter();

	ConvertedShader Convert(uint32_t* shader);
	ConvertedShader mConvertedShader = {};
private:
	std::vector<uint32_t> mInstructions; //used to store the combined instructions for creating a module.

	boost::container::flat_map<D3DSHADER_PARAM_REGISTER_TYPE, boost::container::flat_map<uint32_t, uint32_t> > mRegistersById;
	boost::container::flat_map<D3DSHADER_PARAM_REGISTER_TYPE, boost::container::flat_map<uint32_t, uint32_t> > mIdsByRegister;

	std::vector<uint32_t> mInputRegisters;
	std::vector<uint32_t> mOutputRegisters;
	boost::container::flat_map<_D3DDECLUSAGE, uint32_t> mOutputRegisterUsages;

	//boost::container::flat_map<TypeDescription, uint32_t> mTypeIdPairs;
	boost::container::flat_map<TypeDescription, uint32_t> mTypeIdPairs;
	boost::container::flat_map<uint32_t, TypeDescription> mIdTypePairs;

	std::vector<uint32_t> mCapabilityInstructions;
	std::vector<uint32_t> mExtensionInstructions;
	std::vector<uint32_t> mImportExtendedInstructions;
	std::vector<uint32_t> mMemoryModelInstructions;
	std::vector<uint32_t> mEntryPointInstructions;
	std::vector<uint32_t> mExecutionModeInstructions;

	std::vector<uint32_t> mStringInstructions;
	std::vector<uint32_t> mSourceExtensionInstructions;
	std::vector<uint32_t> mSourceInstructions;
	std::vector<uint32_t> mSourceContinuedInstructions;
	std::vector<uint32_t> mNameInstructions;
	std::vector<uint32_t> mMemberNameInstructions;

	std::vector<uint32_t> mDecorateInstructions;
	std::vector<uint32_t> mMemberDecorateInstructions;
	std::vector<uint32_t> mGroupDecorateInstructions;
	std::vector<uint32_t> mGroupMemberDecorateInstructions;
	std::vector<uint32_t> mDecorationGroupInstructions;

	std::vector<uint32_t> mTypeInstructions;
	std::vector<uint32_t> mFunctionDeclarationInstructions;
	std::vector<uint32_t> mFunctionDefinitionInstructions;

	std::stack<uint32_t> mFalseLabels;
	size_t mFalseLabelCount;

	uint32_t* mBaseToken = nullptr;
	uint32_t* mPreviousToken = nullptr;
	uint32_t* mNextToken = nullptr;
	uint32_t mBaseId = 1;
	uint32_t mNextId = 1;
	uint32_t mTokenOffset = 0;
	uint32_t mMinorVersion = 0;
	uint32_t mMajorVersion = 0;
	uint32_t mPositionRegister = -1;
	uint32_t mEntryPointTypeId = -1;
	uint32_t mEntryPointId = -1;
	bool mIsVertexShader = false;
	uint32_t mPositionId = 0;
	uint32_t mPositionYId = 0;
	uint32_t mGlslExtensionId = 0;

	uint32_t mColor1Id = 0;
	uint32_t mColor1XId = 0;
	uint32_t mColor1YId = 0;
	uint32_t mColor1ZId = 0;
	uint32_t mColor1WId = 0;

	uint32_t mColor2Id = 0;
	uint32_t mColor2XId = 0;
	uint32_t mColor2YId = 0;
	uint32_t mColor2ZId = 0;
	uint32_t mColor2WId = 0;

	uint32_t m255FloatId = 0;
	uint32_t m255VectorId = 0;

	uint32_t m0Id = 0;
	uint32_t m1Id = 0;
	uint32_t m2Id = 0;
	uint32_t m3Id = 0;

	//
	uint32_t mTextures[12] = {};

	Token GetNextToken();
	void SkipTokens(uint32_t numberToSkip);
	uint32_t GetNextId();
	void SkipIds(uint32_t numberToSkip);
	uint32_t GetSpirVTypeId(spv::Op registerType, uint32_t id = UINT_MAX);
	uint32_t GetSpirVTypeId(spv::Op registerType1, spv::Op registerType2);
	uint32_t GetSpirVTypeId(spv::Op registerType1, spv::Op registerType2, uint32_t componentCount);
	uint32_t GetSpirVTypeId(spv::Op registerType1, spv::Op registerType2, spv::Op registerType3, uint32_t componentCount);
	uint32_t GetSpirVTypeId(TypeDescription& registerType, uint32_t id = UINT_MAX);
	uint32_t GetNextVersionId(const Token& token);
	uint32_t GetIdByRegister(const Token& token, _D3DSHADER_PARAM_REGISTER_TYPE type = D3DSPR_FORCE_DWORD, _D3DDECLUSAGE usage = D3DDECLUSAGE_TEXCOORD);
	void SetIdByRegister(const Token& token, uint32_t id);
	TypeDescription GetTypeByRegister(const Token& token, _D3DDECLUSAGE usage = D3DDECLUSAGE_TEXCOORD);
	uint32_t GetSwizzledId(const Token& token, uint32_t inputId = UINT_MAX, _D3DSHADER_PARAM_REGISTER_TYPE type = D3DSPR_FORCE_DWORD, _D3DDECLUSAGE usage = D3DDECLUSAGE_TEXCOORD);
	uint32_t ApplyWriteMask(const Token& token, uint32_t inputId, _D3DDECLUSAGE usage = D3DDECLUSAGE_TEXCOORD);
	void GenerateYFlip();
	void GeneratePushConstant();
	void GeneratePostition();
	void GenerateConstantIndices();
	void GenerateStore(const Token& token, uint32_t inputId);
	void GenerateDecoration(uint32_t registerNumber, uint32_t inputId, _D3DDECLUSAGE usage, bool isInput);
	void Generate255Constants();
	void GenerateConstantBlock();
	void CombineSpirVOpCodes();
	void CreateSpirVModule();

	void PushLoad(uint32_t resultTypeId, uint32_t resultId, uint32_t pointerId);
	void PushStore(uint32_t pointerId, uint32_t objectId);

	void Push(spv::Op code);
	void Push(spv::Op code, uint32_t argument1);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, uint32_t argument4);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, uint32_t argument4, uint32_t argument5);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, GLSLstd450 argument4, uint32_t argument5);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, uint32_t argument4, uint32_t argument5, uint32_t argument6);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, GLSLstd450 argument4, uint32_t argument5, uint32_t argument6);
	void Push(spv::Op code, uint32_t argument1, uint32_t argument2, uint32_t argument3, uint32_t argument4, uint32_t argument5, uint32_t argument6, uint32_t argument7);
	//declare
	void Process_DCL_Pixel();
	void Process_DCL_Vertex();
	void Process_DCL();
	void Process_DEF();
	void Process_DEFI();
	void Process_DEFB();

	//Flow Control Operators
	void Process_IFC();
	void Process_IF();
	void Process_ELSE();
	void Process_ENDIF();

	//Unary Operators
	void Process_NRM();
	void Process_MOV();
	void Process_MOVA();
	void Process_RSQ();

	//Binary Operators (mixed unary in here by mistake will sort out some day ... maybe)
	void Process_DST();
	void Process_CRS();
	void Process_POW();
	void Process_MUL();
	void Process_EXP();
	void Process_EXPP();
	void Process_LOG();
	void Process_LOGP();
	void Process_FRC();
	void Process_ABS();
	void Process_ADD();
	void Process_SUB();
	void Process_MIN();
	void Process_MAX();
	void Process_DP3();
	void Process_DP4();
	void Process_TEX();
	void Process_TEXCOORD();

	//matrix operators
	void Process_M4x4();
	void Process_M4x3();
	void Process_M3x4();
	void Process_M3x3();
	void Process_M3x2();

	//ternary operators
	void Process_MAD();
};

#endif //SHADERCONVERTER_H
//
