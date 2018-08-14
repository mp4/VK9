/*
Copyright(c) 2018 Christopher Joseph Dean Schaefer

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


#include <chrono>
#include "RealDevice.h"

#ifndef DRAWCONTEXT_H
#define DRAWCONTEXT_H

struct DrawContext
{
	//Vulkan State
	vk::DescriptorSetLayout DescriptorSetLayout;
	vk::Pipeline Pipeline;
	vk::PipelineLayout PipelineLayout;

	//Misc
	//boost::container::flat_map<UINT, UINT> Bindings;
	UINT Bindings[64] = {};

	//D3D9 State - Pipe
	D3DPRIMITIVETYPE PrimitiveType = D3DPT_FORCE_DWORD;
	DWORD FVF = 0;
	CVertexDeclaration9* VertexDeclaration = nullptr;
	CVertexShader9* VertexShader = nullptr;
	CPixelShader9* PixelShader = nullptr;
	int32_t StreamCount = 0;

	//D3d9 State - Lights
	ShaderConstantSlots mVertexShaderConstantSlots = {};
	ShaderConstantSlots mPixelShaderConstantSlots = {};

	//Constant Registers
	SpecializationConstants mSpecializationConstants = {};

	//Resource Handling.
	std::chrono::steady_clock::time_point LastUsed = std::chrono::steady_clock::now();
	RealDevice* mRealDevice = nullptr; //null if not owner.
	DrawContext(RealDevice* realDevice) : mRealDevice(realDevice) {}
	~DrawContext();
};

#endif //DRAWCONTEXT_H
