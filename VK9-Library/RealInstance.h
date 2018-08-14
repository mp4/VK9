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

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>
#include <boost/container/small_vector.hpp>

#ifdef _DEBUG
#include "renderdoc_app.h"
#endif // _DEBUG

#ifndef REALINSTANCE_H
#define REALINSTANCE_H

struct RealInstance
{
	vk::Instance mInstance;
	vk::PhysicalDevice* mPhysicalDevices = nullptr;
	uint32_t mPhysicalDeviceCount = 0;

	//boost::container::small_vector<std::shared_ptr<RealDevice>, 1> mDevices;

#ifdef _DEBUG
	HMODULE mRenderDocDll = nullptr;
	RENDERDOC_API_1_1_1* mRenderDocApi = nullptr;
	vk::DebugReportCallbackEXT mCallback;
#endif // _DEBUG

	RealInstance();
	~RealInstance();
};

#endif // REALINSTANCE_H