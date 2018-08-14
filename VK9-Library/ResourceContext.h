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
#include <vulkan/vulkan.hpp>
#include "RealDevice.h"

#ifndef RESOURCECONTEXT_H
#define RESOURCECONTEXT_H

struct ResourceContext
{
	vk::DescriptorImageInfo DescriptorImageInfo[16] = {};

	//Vulkan State
	vk::DescriptorSetLayout DescriptorSetLayout;
	vk::PipelineLayout PipelineLayout;
	vk::DescriptorSet DescriptorSet;
	BOOL WasShader = false; // descriptor set logic is different for shaders so mixing them makes Vulkan angry because the number of attachment is different and stuff.

							//Resource Handling.
	std::chrono::steady_clock::time_point LastUsed = std::chrono::steady_clock::now();
	RealDevice* mRealDevice = nullptr; //null if not owner.
	ResourceContext(RealDevice* realDevice) : mRealDevice(realDevice) {}
	~ResourceContext();
};

#endif //RESOURCECONTEXT_H