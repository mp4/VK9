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

#ifndef CTYPES_H
#define CTYPES_H

#include "d3d9.h"
#include "d3d9types.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <boost/container/small_vector.hpp>
#include <boost/container/flat_map.hpp>

#include <Eigen/Dense>

class CVertexBuffer9;
class CVertexDeclaration9;
class CIndexBuffer9;
class CPixelShader9;
class CVertexShader9;
class CTexture9;

struct Monitor
{
	HMONITOR hMonitor = NULL;
	HDC hdcMonitor = NULL;
	uint32_t Height = 0;
	uint32_t Width = 0;
	uint32_t RefreshRate = 0;
	uint32_t PixelBits = 0;
	uint32_t ColorPlanes = 0;

};

struct Vertex
{
	float x, y, z; // Position of vertex in 3D space
	DWORD color;   // Color of vertex
};

/*
The structure below is using vec4 for position and direction because some implementations handle vec3 strange.
The alignment requirements are different for vec4 versus int/float so I moved "Type" after the vec4 variables to prevent strange padding in the middle of the structure.
In addition if the structure isn't a multiple of 16 the stride between elements in an array can be different. That is why the filler variables are included.
This structure is exactly 128 bytes.
*/
struct Light
{
	float                 Diffuse[4] = {};         /* Diffuse color of light */
	float                 Specular[4] = {};        /* Specular color of light */
	float                 Ambient[4] = {};         /* Ambient color of light */
	float                 Position[4] = {};        /* Position in world space */
	float                 Direction[4] = {};       /* Direction in world space */
	int                   Type = 0;            /* Type of light source */
	float                 Range = 0;           /* Cutoff range */
	float                 Falloff = 0;         /* Falloff */
	float                 Attenuation0 = 0;    /* Constant attenuation */
	float                 Attenuation1 = 0;    /* Linear attenuation */
	float                 Attenuation2 = 0;    /* Quadratic attenuation */
	float                 Theta = 0;           /* Inner angle of spotlight cone */
	float                 Phi = 0;             /* Outer angle of spotlight cone */
	int                   IsEnabled = 0;       /*Replaces separate enable structure.*/
	int                   Filler1 = 0;
	int                   Filler2 = 0;
	int                   Filler3 = 0;
};

class StreamSource
{
public:

	UINT StreamNumber;
	CVertexBuffer9* StreamData;
	VkDeviceSize OffsetInBytes;
	UINT Stride;

	StreamSource();
	StreamSource(const StreamSource& value);
	StreamSource& operator =(const StreamSource& value);

	StreamSource(UINT streamNumber, CVertexBuffer9* streamData, VkDeviceSize offsetInBytes, UINT stride);
	~StreamSource();
};

struct ShaderConstantSlots
{
	uint32_t IntegerConstants[16 * 4]; //= { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
	BOOL BooleanConstants[16]; //= { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
	float FloatConstants[256 * 4]; //= { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
};

struct SpecializationConstants
{
	int lightCount = 1;
	int placeholder1 = 0;
	int placeholder2 = 0;
	//int shadeMode = D3DSHADE_GOURAUD;
	//int isLightingEnabled = true;
	int textureCount = 1;

	//Texture Stage _0
	int Constant_0 = 0;
	int Result_0 = D3DTA_CURRENT;
	int textureTransformationFlags_0 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_0 = 0;
	int colorOperation_0 = D3DTOP_MODULATE;
	int colorArgument0_0 = D3DTA_CURRENT;
	int colorArgument1_0 = D3DTA_TEXTURE;
	int colorArgument2_0 = D3DTA_CURRENT;
	int alphaOperation_0 = D3DTOP_SELECTARG1;
	int alphaArgument0_0 = D3DTA_CURRENT;
	int alphaArgument1_0 = D3DTA_TEXTURE;
	int alphaArgument2_0 = D3DTA_CURRENT;
	float bumpMapMatrix00_0 = 0.0f;
	float bumpMapMatrix01_0 = 0.0f;
	float bumpMapMatrix10_0 = 0.0f;
	float bumpMapMatrix11_0 = 0.0f;
	float bumpMapScale_0 = 0.0f;
	float bumpMapOffset_0 = 0.0f;

	//Texture Stage _1
	int Constant_1 = 0;
	int Result_1 = D3DTA_CURRENT;
	int textureTransformationFlags_1 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_1 = 1;
	int colorOperation_1 = D3DTOP_DISABLE;
	int colorArgument0_1 = D3DTA_CURRENT;
	int colorArgument1_1 = D3DTA_TEXTURE;
	int colorArgument2_1 = D3DTA_CURRENT;
	int alphaOperation_1 = D3DTOP_DISABLE;
	int alphaArgument0_1 = D3DTA_CURRENT;
	int alphaArgument1_1 = D3DTA_TEXTURE;
	int alphaArgument2_1 = D3DTA_CURRENT;
	float bumpMapMatrix00_1 = 0.0f;
	float bumpMapMatrix01_1 = 0.0f;
	float bumpMapMatrix10_1 = 0.0f;
	float bumpMapMatrix11_1 = 0.0f;
	float bumpMapScale_1 = 0.0f;
	float bumpMapOffset_1 = 0.0f;

	//Texture Stage _2
	int Constant_2 = 0;
	int Result_2 = D3DTA_CURRENT;
	int textureTransformationFlags_2 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_2 = 2;
	int colorOperation_2 = D3DTOP_DISABLE;
	int colorArgument0_2 = D3DTA_CURRENT;
	int colorArgument1_2 = D3DTA_TEXTURE;
	int colorArgument2_2 = D3DTA_CURRENT;
	int alphaOperation_2 = D3DTOP_DISABLE;
	int alphaArgument0_2 = D3DTA_CURRENT;
	int alphaArgument1_2 = D3DTA_TEXTURE;
	int alphaArgument2_2 = D3DTA_CURRENT;
	float bumpMapMatrix00_2 = 0.0f;
	float bumpMapMatrix01_2 = 0.0f;
	float bumpMapMatrix10_2 = 0.0f;
	float bumpMapMatrix11_2 = 0.0f;
	float bumpMapScale_2 = 0.0f;
	float bumpMapOffset_2 = 0.0f;

	//Texture Stage _3
	int Constant_3 = 0;
	int Result_3 = D3DTA_CURRENT;
	int textureTransformationFlags_3 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_3 = 3;
	int colorOperation_3 = D3DTOP_DISABLE;
	int colorArgument0_3 = D3DTA_CURRENT;
	int colorArgument1_3 = D3DTA_TEXTURE;
	int colorArgument2_3 = D3DTA_CURRENT;
	int alphaOperation_3 = D3DTOP_DISABLE;
	int alphaArgument0_3 = D3DTA_CURRENT;
	int alphaArgument1_3 = D3DTA_TEXTURE;
	int alphaArgument2_3 = D3DTA_CURRENT;
	float bumpMapMatrix00_3 = 0.0f;
	float bumpMapMatrix01_3 = 0.0f;
	float bumpMapMatrix10_3 = 0.0f;
	float bumpMapMatrix11_3 = 0.0f;
	float bumpMapScale_3 = 0.0f;
	float bumpMapOffset_3 = 0.0f;

	//Texture Stage _4
	int Constant_4 = 0;
	int Result_4 = D3DTA_CURRENT;
	int textureTransformationFlags_4 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_4 = 4;
	int colorOperation_4 = D3DTOP_DISABLE;
	int colorArgument0_4 = D3DTA_CURRENT;
	int colorArgument1_4 = D3DTA_TEXTURE;
	int colorArgument2_4 = D3DTA_CURRENT;
	int alphaOperation_4 = D3DTOP_DISABLE;
	int alphaArgument0_4 = D3DTA_CURRENT;
	int alphaArgument1_4 = D3DTA_TEXTURE;
	int alphaArgument2_4 = D3DTA_CURRENT;
	float bumpMapMatrix00_4 = 0.0f;
	float bumpMapMatrix01_4 = 0.0f;
	float bumpMapMatrix10_4 = 0.0f;
	float bumpMapMatrix11_4 = 0.0f;
	float bumpMapScale_4 = 0.0f;
	float bumpMapOffset_4 = 0.0f;

	//Texture Stage _5
	int Constant_5 = 0;
	int Result_5 = D3DTA_CURRENT;
	int textureTransformationFlags_5 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_5 = 5;
	int colorOperation_5 = D3DTOP_DISABLE;
	int colorArgument0_5 = D3DTA_CURRENT;
	int colorArgument1_5 = D3DTA_TEXTURE;
	int colorArgument2_5 = D3DTA_CURRENT;
	int alphaOperation_5 = D3DTOP_DISABLE;
	int alphaArgument0_5 = D3DTA_CURRENT;
	int alphaArgument1_5 = D3DTA_TEXTURE;
	int alphaArgument2_5 = D3DTA_CURRENT;
	float bumpMapMatrix00_5 = 0.0f;
	float bumpMapMatrix01_5 = 0.0f;
	float bumpMapMatrix10_5 = 0.0f;
	float bumpMapMatrix11_5 = 0.0f;
	float bumpMapScale_5 = 0.0f;
	float bumpMapOffset_5 = 0.0f;

	//Texture Stage _6
	int Constant_6 = 0;
	int Result_6 = D3DTA_CURRENT;
	int textureTransformationFlags_6 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_6 = 6;
	int colorOperation_6 = D3DTOP_DISABLE;
	int colorArgument0_6 = D3DTA_CURRENT;
	int colorArgument1_6 = D3DTA_TEXTURE;
	int colorArgument2_6 = D3DTA_CURRENT;
	int alphaOperation_6 = D3DTOP_DISABLE;
	int alphaArgument0_6 = D3DTA_CURRENT;
	int alphaArgument1_6 = D3DTA_TEXTURE;
	int alphaArgument2_6 = D3DTA_CURRENT;
	float bumpMapMatrix00_6 = 0.0f;
	float bumpMapMatrix01_6 = 0.0f;
	float bumpMapMatrix10_6 = 0.0f;
	float bumpMapMatrix11_6 = 0.0f;
	float bumpMapScale_6 = 0.0f;
	float bumpMapOffset_6 = 0.0f;

	//Texture Stage _7
	int Constant_7 = 0;
	int Result_7 = D3DTA_CURRENT;
	int textureTransformationFlags_7 = D3DTTFF_DISABLE;
	int texureCoordinateIndex_7 = 7;
	int colorOperation_7 = D3DTOP_DISABLE;
	int colorArgument0_7 = D3DTA_CURRENT;
	int colorArgument1_7 = D3DTA_TEXTURE;
	int colorArgument2_7 = D3DTA_CURRENT;
	int alphaOperation_7 = D3DTOP_DISABLE;
	int alphaArgument0_7 = D3DTA_CURRENT;
	int alphaArgument1_7 = D3DTA_TEXTURE;
	int alphaArgument2_7 = D3DTA_CURRENT;
	float bumpMapMatrix00_7 = 0.0f;
	float bumpMapMatrix01_7 = 0.0f;
	float bumpMapMatrix10_7 = 0.0f;
	float bumpMapMatrix11_7 = 0.0f;
	float bumpMapScale_7 = 0.0f;
	float bumpMapOffset_7 = 0.0f;

	//Render State
	int zEnable = D3DZB_TRUE;
	int fillMode = D3DFILL_SOLID;
	int shadeMode = D3DSHADE_GOURAUD;
	int zWriteEnable = true;
	int alphaTestEnable = false;
	int lastPixel = true;
	int sourceBlend = D3DBLEND_ONE;
	int destinationBlend = D3DBLEND_ZERO;
	int cullMode = D3DCULL_CCW; // D3DCULL_CW;
	int zFunction = D3DCMP_LESSEQUAL;
	int alphaReference = 0;
	int alphaFunction = D3DCMP_ALWAYS;
	int ditherEnable = false;
	int alphaBlendEnable = false;
	int fogEnable = false;
	int specularEnable = false;
	uint32_t fogColor = 0;
	int fogTableMode = D3DFOG_NONE;
	float fogStart = 0.0f;
	float fogEnd = 1.0f;
	float fogDensity = 1.0f;
	int rangeFogEnable = false;
	int stencilEnable = false;
	int stencilFail = D3DSTENCILOP_KEEP;
	int stencilZFail = D3DSTENCILOP_KEEP;
	int stencilPass = D3DSTENCILOP_KEEP;
	int stencilFunction = D3DCMP_ALWAYS;
	int stencilReference = 0;
	uint32_t stencilMask = 0xFFFFFFFF;
	uint32_t stencilWriteMask = 0xFFFFFFFF;
	uint32_t textureFactor = 0xFFFFFFFF;
	int wrap0 = 0;
	int wrap1 = 0;
	int wrap2 = 0;
	int wrap3 = 0;
	int wrap4 = 0;
	int wrap5 = 0;
	int wrap6 = 0;
	int wrap7 = 0;
	int clipping = true;
	int lighting = true;
	uint32_t ambient = 0;
	int fogVertexMode = D3DFOG_NONE;
	int colorVertex = true;
	int localViewer = true;
	int normalizeNormals = false;
	int diffuseMaterialSource = D3DMCS_COLOR1;
	int specularMaterialSource = D3DMCS_COLOR2;
	int ambientMaterialSource = D3DMCS_MATERIAL;
	int emissiveMaterialSource = D3DMCS_MATERIAL;
	int vertexBlend = D3DVBF_DISABLE;
	int clipPlaneEnable = 0;
	float pointSize = 64.0f;
	float pointSizeMinimum = 1.0f;
	int pointSpriteEnable = false;
	int pointScaleEnable = false;
	float pointScaleA = 1.0f;
	float pointScaleB = 0.0f;
	float pointScaleC = 0.0f;
	int multisampleAntiAlias = true;
	uint32_t multisampleMask = 0xFFFFFFFF;
	int patchEdgeStyle = D3DPATCHEDGE_DISCRETE;
	int debugMonitorToken = D3DDMT_ENABLE;
	float pointSizeMaximum = 64.0f;
	int indexedVertexBlendEnable = false;
	int colorWriteEnable = 0x0000000F;
	float tweenFactor = 0.0f;
	int blendOperation = D3DBLENDOP_ADD;
	int positionDegree = D3DDEGREE_CUBIC;
	int normalDegree = D3DDEGREE_LINEAR;
	int scissorTestEnable = false;
	float slopeScaleDepthBias = 0.00f;
	int antiAliasedLineEnable = false;
	float minimumTessellationLevel = 1.0f;
	float maximumTessellationLevel = 1.0f;
	float adaptivetessX = 0.0f;
	float adaptivetessY = 0.0f;
	float adaptivetessZ = 1.0f;
	float adaptivetessW = 0.0f;
	int enableAdaptiveTessellation = false;
	int twoSidedStencilMode = false;
	int ccwStencilFail = D3DSTENCILOP_KEEP;
	int ccwStencilZFail = D3DSTENCILOP_KEEP;
	int ccwStencilPass = D3DSTENCILOP_KEEP;
	int ccwStencilFunction = D3DCMP_ALWAYS;
	int colorWriteEnable1 = 0x0000000f;
	int colorWriteEnable2 = 0x0000000f;
	int colorWriteEnable3 = 0x0000000f;
	uint32_t blendFactor = 0xffffffff;
	int srgbWriteEnable = 0;
	float depthBias = 0.00f;
	int wrap8 = 0;
	int wrap9 = 0;
	int wrap10 = 0;
	int wrap11 = 0;
	int wrap12 = 0;
	int wrap13 = 0;
	int wrap14 = 0;
	int wrap15 = 0;
	int separateAlphaBlendEnable = false;
	int sourceBlendAlpha = D3DBLEND_ONE;
	int destinationBlendAlpha = D3DBLEND_ZERO;
	int blendOperationAlpha = D3DBLENDOP_ADD;
	int screenWidth = 640;
	int screenHeight = 480;
};

struct RealIndexBuffer;
struct RealRenderTarget;

struct DeviceState
{
	//IDirect3DDevice9::LightEnable
	//IDirect3DDevice9::SetClipPlane
	//IDirect3DDevice9::SetCurrentTexturePalette
	//IDirect3DDevice9::SetFVF
	DWORD mFVF = LONG_MAX;
	BOOL mHasFVF = 0;

	//IDirect3DDevice9::SetIndices
	CIndexBuffer9* mOriginalIndexBuffer = nullptr;
	RealIndexBuffer* mIndexBuffer = nullptr;
	BOOL mHasIndexBuffer = 0;
	std::shared_ptr<RealRenderTarget> mRenderTarget;

	//IDirect3DDevice9::SetLight
	//boost::container::small_vector<Light, 4> mLights;
	std::vector<Light> mLights;
	BOOL mAreLightsDirty = true;

	//IDirect3DDevice9::SetMaterial
	D3DMATERIAL9 mMaterial = 
	{
		/*1.0f,1.0f,1.0f,1.0f,
		1.0f,1.0f,1.0f,1.0f,
		1.0f,1.0f,1.0f,1.0f,
		1.0f,1.0f,1.0f,1.0f,
		1.0f*/
	};
	BOOL mIsMaterialDirty = true;

	//IDirect3DDevice9::SetNPatchMode
	float mNSegments = -1;

	//IDirect3DDevice9::SetPixelShader
	CPixelShader9* mPixelShader = nullptr;
	BOOL mHasPixelShader = 0;

	//IDirect3DDevice9::SetPixelShaderConstantB
	//IDirect3DDevice9::SetPixelShaderConstantF
	//IDirect3DDevice9::SetPixelShaderConstantI
	//IDirect3DDevice9::SetRenderState
	//Render State
	bool hasZEnable = false;
	bool hasFillMode = false;
	bool hasShadeMode = false;
	bool hasZWriteEnable = false;
	bool hasAlphaTestEnable = false;
	bool hasLastPixel = false;
	bool hasSourceBlend = false;
	bool hasDestinationBlend = false;
	bool hasCullMode = false;
	bool hasZFunction = false;
	bool hasAlphaReference = false;
	bool hasAlphaFunction = false;
	bool hasDitherEnable = false;
	bool hasAlphaBlendEnable = false;
	bool hasFogEnable = false;
	bool hasSpecularEnable = false;
	bool hasFogColor = false;
	bool hasFogTableMode = false;
	bool hasFogStart = false;
	bool hasFogEnd = false;
	bool hasFogDensity = false;
	bool hasRangeFogEnable = false;
	bool hasStencilEnable = false;
	bool hasStencilFail = false;
	bool hasStencilZFail = false;
	bool hasStencilPass = false;
	bool hasStencilFunction = false;
	bool hasStencilReference = false;
	bool hasStencilMask = false;
	bool hasStencilWriteMask = false;
	bool hasTextureFactor = false;
	bool hasWrap0 = false;
	bool hasWrap1 = false;
	bool hasWrap2 = false;
	bool hasWrap3 = false;
	bool hasWrap4 = false;
	bool hasWrap5 = false;
	bool hasWrap6 = false;
	bool hasWrap7 = false;
	bool hasClipping = false;
	bool hasLighting = false;
	bool hasAmbient = false;
	bool hasFogVertexMode = false;
	bool hasColorVertex = false;
	bool hasLocalViewer = false;
	bool hasNormalizeNormals = false;
	bool hasDiffuseMaterialSource = false;
	bool hasSpecularMaterialSource = false;
	bool hasAmbientMaterialSource = false;
	bool hasEmissiveMaterialSource = false;
	bool hasVertexBlend = false;
	bool hasClipPlaneEnable = false;
	bool hasPointSize = false;
	bool hasPointSizeMinimum = false;
	bool hasPointSpriteEnable = false;
	bool hasPointScaleEnable = false;
	bool hasPointScaleA = false;
	bool hasPointScaleB = false;
	bool hasPointScaleC = false;
	bool hasMultisampleAntiAlias = false;
	bool hasMultisampleMask = false;
	bool hasPatchEdgeStyle = false;
	bool hasDebugMonitorToken = false;
	bool hasPointSizeMaximum = false;
	bool hasIndexedVertexBlendEnable = false;
	bool hasColorWriteEnable = false;
	bool hasTweenFactor = false;
	bool hasBlendOperation = false;
	bool hasPositionDegree = false;
	bool hasNormalDegree = false;
	bool hasScissorTestEnable = false;
	bool hasSlopeScaleDepthBias = false;
	bool hasAntiAliasedLineEnable = false;
	bool hasMinimumTessellationLevel = false;
	bool hasMaximumTessellationLevel = false;
	bool hasAdaptivetessX = false;
	bool hasAdaptivetessY = false;
	bool hasAdaptivetessZ = false;
	bool hasAdaptivetessW = false;
	bool hasEnableAdaptiveTessellation = false;
	bool hasTwoSidedStencilMode = false;
	bool hasCcwStencilFail = false;
	bool hasCcwStencilZFail = false;
	bool hasCcwStencilPass = false;
	bool hasCcwStencilFunction = false;
	bool hasColorWriteEnable1 = false;
	bool hasColorWriteEnable2 = false;
	bool hasColorWriteEnable3 = false;
	bool hasBlendFactor = false;
	bool hasSrgbWriteEnable = false;
	bool hasDepthBias = false;
	bool hasWrap8 = false;
	bool hasWrap9 = false;
	bool hasWrap10 = false;
	bool hasWrap11 = false;
	bool hasWrap12 = false;
	bool hasWrap13 = false;
	bool hasWrap14 = false;
	bool hasWrap15 = false;
	bool hasSeparateAlphaBlendEnable = false;
	bool hasSourceBlendAlpha = false;
	bool hasDestinationBlendAlpha = false;
	bool hasBlendOperationAlpha = false;

	//IDirect3DDevice9::SetSamplerState
	boost::container::flat_map<DWORD, boost::container::flat_map<D3DSAMPLERSTATETYPE, DWORD> > mSamplerStates;

	//IDirect3DDevice9::SetScissorRect
	RECT m9Scissor = {};
	vk::Rect2D mScissor;

	//IDirect3DDevice9::SetStreamSource
	boost::container::flat_map<UINT, StreamSource> mStreamSources;

	//IDirect3DDevice9::SetStreamSourceFreq
	//IDirect3DDevice9::SetTexture
	vk::DescriptorImageInfo mDescriptorImageInfo[16];
	//boost::container::flat_map<DWORD, IDirect3DBaseTexture9*> mTextures;
	IDirect3DBaseTexture9* mTextures[16] = {};

	//IDirect3DDevice9::SetTextureStageState
	//boost::container::flat_map<DWORD, boost::container::flat_map<D3DTEXTURESTAGESTATETYPE, DWORD> > mTextureStageStates;

	//IDirect3DDevice9::SetTransform
	boost::container::flat_map<D3DTRANSFORMSTATETYPE, D3DMATRIX> mTransforms;
	BOOL mHasTransformsChanged = true;

	//IDirect3DDevice9::SetViewport
	D3DVIEWPORT9 m9Viewport = {};
	vk::Viewport mViewport;

	//IDirect3DDevice9::SetVertexDeclaration
	CVertexDeclaration9* mVertexDeclaration = nullptr;
	BOOL mHasVertexDeclaration = 0;

	//IDirect3DDevice9::SetVertexShader
	CVertexShader9* mVertexShader = nullptr;
	BOOL mHasVertexShader = 0;

	//IDirect3DDevice9::SetVertexShaderConstantB
	//IDirect3DDevice9::SetVertexShaderConstantF
	//IDirect3DDevice9::SetVertexShaderConstantI

	//Used for shader specialization.
	SpecializationConstants mSpecializationConstants = {};

	float mPushConstants[64] = {};
	ShaderConstantSlots mVertexShaderConstantSlots;
	ShaderConstantSlots mPixelShaderConstantSlots;

	//ConvertedShader mConvertedVertexShader = {};
	//ConvertedShader mConvertedPixelShader = {};

	bool hasPresented = true;
};

struct color_A8R8G8B8
{
	unsigned char B = 0;
	unsigned char G = 0;
	unsigned char R = 0;
	unsigned char A = 0;
};

struct Transformations
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	Eigen::Matrix<float, 4, 4, Eigen::DontAlign> mTotalTransformation;
	Eigen::Matrix<float, 4, 4, Eigen::DontAlign> mModel;
	Eigen::Matrix<float, 4, 4, Eigen::DontAlign> mView;
	Eigen::Matrix<float, 4, 4, Eigen::DontAlign> mProjection;
};

union PushConstants
{
	float Floats[64] = {};
	uint32_t Integers[64];
	BOOL Booleans[64];
};

#endif // CTYPES_H
