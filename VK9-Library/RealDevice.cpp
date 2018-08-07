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

#include "RealDevice.h"
#include "RealRenderTarget.h"

RealDevice::RealDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, int32_t width, int32_t height)
	: mInstance(instance),
	mPhysicalDevice(physicalDevice)
{
	BOOST_LOG_TRIVIAL(info) << "RealDevice::RealDevice";

	vk::Result result;

	//Grab the properties for GetAdapterIdentifier and other calls.
	physicalDevice.getProperties(&mPhysicalDeviceProperties);

	//Grab the features for GetDeviceCaps and other calls.
	physicalDevice.getFeatures(&mPhysicalDeviceFeatures);

	//Grab the memory properties for CDevice init and other calls.
	physicalDevice.getMemoryProperties(&mPhysicalDeviceMemoryProperties);

	//QueueFamilyProperties
	physicalDevice.getQueueFamilyProperties(&mQueueFamilyPropertyCount, nullptr);
	mQueueFamilyProperties = new vk::QueueFamilyProperties[mQueueFamilyPropertyCount];
	physicalDevice.getQueueFamilyProperties(&mQueueFamilyPropertyCount, mQueueFamilyProperties);

	boost::container::small_vector<char*, 16> extensionNames;
	boost::container::small_vector<char*, 16> layerNames;

	extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//extensionNames.push_back("VK_KHR_maintenance1");
	//extensionNames.push_back("VK_KHR_push_descriptor");
	//extensionNames.push_back("VK_KHR_sampler_mirror_clamp_to_edge");
#ifdef _DEBUG
	layerNames.push_back("VK_LAYER_LUNARG_standard_validation");
#endif // _DEBUG

	float queue_priorities[1] = { 0.0 };
	vk::DeviceQueueCreateInfo queue_info = {};
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = queue_priorities;

	vk::DeviceCreateInfo device_info = {};
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = extensionNames.size();
	device_info.ppEnabledExtensionNames = extensionNames.data();
	device_info.enabledLayerCount = layerNames.size();
	device_info.ppEnabledLayerNames = layerNames.data();
	device_info.pEnabledFeatures = &mPhysicalDeviceFeatures; //Enable all available because we don't know ahead of time what features will be used.

	result = physicalDevice.createDevice(&device_info, nullptr, &mDevice);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateDevice failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::DescriptorPoolSize descriptorPoolSizes[11] = {};
	descriptorPoolSizes[0].type = vk::DescriptorType::eSampler; //VK_DESCRIPTOR_TYPE_SAMPLER;
	descriptorPoolSizes[0].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetSamplers);
	descriptorPoolSizes[1].type = vk::DescriptorType::eCombinedImageSampler; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxPerStageDescriptorSamplers);
	descriptorPoolSizes[2].type = vk::DescriptorType::eSampledImage; //VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorPoolSizes[2].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetSampledImages);
	descriptorPoolSizes[3].type = vk::DescriptorType::eStorageImage; //VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorPoolSizes[3].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetStorageImages);
	descriptorPoolSizes[4].type = vk::DescriptorType::eUniformTexelBuffer; //VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	descriptorPoolSizes[4].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxPerStageDescriptorSampledImages);
	descriptorPoolSizes[5].type = vk::DescriptorType::eStorageTexelBuffer; //VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	descriptorPoolSizes[5].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxPerStageDescriptorStorageImages);
	descriptorPoolSizes[6].type = vk::DescriptorType::eUniformBuffer; //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[6].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
	descriptorPoolSizes[7].type = vk::DescriptorType::eStorageBuffer; //VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[7].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffers);
	descriptorPoolSizes[8].type = vk::DescriptorType::eUniformBufferDynamic; //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSizes[8].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
	descriptorPoolSizes[9].type = vk::DescriptorType::eStorageBufferDynamic; //VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	descriptorPoolSizes[9].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic);
	descriptorPoolSizes[10].type = vk::DescriptorType::eInputAttachment; //VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	descriptorPoolSizes[10].descriptorCount = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetInputAttachments);

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.maxSets = std::min((uint32_t)MAX_DESCRIPTOR, mPhysicalDeviceProperties.limits.maxDescriptorSetSamplers);
	descriptorPoolCreateInfo.poolSizeCount = 11;
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
	descriptorPoolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet; //This flag allows descriptors to return to the pool when they are freed.

	result = mDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &mDescriptorPool);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateDescriptorPool failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	for (size_t i = 0; i < mPhysicalDeviceMemoryProperties.memoryHeapCount; i++)
	{
		mEstimatedMemory += mPhysicalDeviceMemoryProperties.memoryHeaps[i].size;
	}
	BOOST_LOG_TRIVIAL(info) << "RealDevice::RealDevice The total size of all heaps is " << mEstimatedMemory;

	vk::Bool32 doesSupportGraphics = false;
	uint32_t graphicsQueueIndex = 0;
	for (uint32_t i = 0; i < mQueueFamilyPropertyCount; i++)
	{
		doesSupportGraphics = (mQueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits(0);
		if (doesSupportGraphics)
		{
			graphicsQueueIndex = i;
			break;
		}
	}

	/*
	Now we need to setup our queues and buffers.
	We'll start with a command pool because that is where we get command buffers from.
	*/
	vk::CommandPoolCreateInfo commandPoolInfo;
	commandPoolInfo.queueFamilyIndex = graphicsQueueIndex; //Found earlier.
	commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	result = mDevice.createCommandPool(&commandPoolInfo, nullptr, &mCommandPool);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateCommandPool failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	//Create queue so we can submit command buffers.
	mDevice.getQueue(graphicsQueueIndex, 0, &mQueue); //no result?

	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandPool = mCommandPool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = 2;

	result = mDevice.allocateCommandBuffers(&commandBufferInfo, mCommandBuffers);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkAllocateCommandBuffers failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	//Load fixed function shaders.
	mVertShaderModule_XYZRHW = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW.vert.spv");
	mVertShaderModule_XYZ = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ.vert.spv");
	mFragShaderModule_XYZ = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ.frag.spv");

	mVertShaderModule_XYZRHW_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW_DIFFUSE.vert.spv");
	mVertShaderModule_XYZ_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE.vert.spv");
	mFragShaderModule_XYZ_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE.frag.spv");
	mGeomShaderModule_XYZ_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE.geom.spv");

	mVertShaderModule_XYZRHW_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW_TEX1.vert.spv");
	mVertShaderModule_XYZ_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_TEX1.vert.spv");
	mFragShaderModule_XYZ_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_TEX1.frag.spv");

	mVertShaderModule_XYZRHW_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW_TEX2.vert.spv");
	mVertShaderModule_XYZ_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_TEX2.vert.spv");
	mFragShaderModule_XYZ_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_TEX2.frag.spv");

	mVertShaderModule_XYZRHW_DIFFUSE_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW_DIFFUSE_TEX1.vert.spv");
	mVertShaderModule_XYZ_DIFFUSE_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE_TEX1.vert.spv");
	mFragShaderModule_XYZ_DIFFUSE_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE_TEX1.frag.spv");

	mVertShaderModule_XYZRHW_DIFFUSE_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZRHW_DIFFUSE_TEX2.vert.spv");
	mVertShaderModule_XYZ_DIFFUSE_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE_TEX2.vert.spv");
	mFragShaderModule_XYZ_DIFFUSE_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_DIFFUSE_TEX2.frag.spv");

	mVertShaderModule_XYZ_NORMAL = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL.vert.spv");
	mFragShaderModule_XYZ_NORMAL = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL.frag.spv");

	mVertShaderModule_XYZ_NORMAL_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_TEX1.vert.spv");
	mFragShaderModule_XYZ_NORMAL_TEX1 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_TEX1.frag.spv");

	mVertShaderModule_XYZ_NORMAL_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_TEX2.vert.spv");
	mFragShaderModule_XYZ_NORMAL_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_TEX2.frag.spv");

	mVertShaderModule_XYZ_NORMAL_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_DIFFUSE.vert.spv");
	mFragShaderModule_XYZ_NORMAL_DIFFUSE = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_DIFFUSE.frag.spv");

	mVertShaderModule_XYZ_NORMAL_DIFFUSE_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_DIFFUSE_TEX2.vert.spv");
	mFragShaderModule_XYZ_NORMAL_DIFFUSE_TEX2 = LoadShaderFromFile(mDevice, "VertexBuffer_XYZ_NORMAL_DIFFUSE_TEX2.frag.spv");

	//pipeline stuff.
	mPushConstantRanges[0].offset = 0;
	mPushConstantRanges[0].size = UBO_SIZE * 2; //There are 2 matrices one for world transform and one for all transforms.
	mPushConstantRanges[0].stageFlags = vk::ShaderStageFlagBits::eAllGraphics; //VK_SHADER_STAGE_ALL_GRAPHICS

	mVertexSpecializationInfo.pData = &mDeviceState.mSpecializationConstants;
	mVertexSpecializationInfo.dataSize = sizeof(SpecializationConstants);

	mPixelSpecializationInfo.pData = &mDeviceState.mSpecializationConstants;
	mPixelSpecializationInfo.dataSize = sizeof(SpecializationConstants);

	mPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1; //reset later.
	mPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = mVertexInputBindingDescription;
	mPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2; //reset later.
	mPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = mVertexInputAttributeDescription;

	mDynamicStateEnables[mPipelineDynamicStateCreateInfo.dynamicStateCount++] = vk::DynamicState::eViewport;
	mDynamicStateEnables[mPipelineDynamicStateCreateInfo.dynamicStateCount++] = vk::DynamicState::eScissor;
	mDynamicStateEnables[mPipelineDynamicStateCreateInfo.dynamicStateCount++] = vk::DynamicState::eDepthBias;
	mPipelineDynamicStateCreateInfo.pDynamicStates = mDynamicStateEnables;

	mPipelineRasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
	mPipelineRasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
	mPipelineRasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
	mPipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	mPipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	mPipelineRasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
	mPipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	mPipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	mPipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	mPipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;

	mPipelineInputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;

	//mPipelineColorBlendAttachmentState[0].colorWriteMask = 0xf;
	mPipelineColorBlendAttachmentState[0].blendEnable = VK_TRUE;
	mPipelineColorBlendAttachmentState[0].colorBlendOp = vk::BlendOp::eAdd;
	mPipelineColorBlendAttachmentState[0].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	mPipelineColorBlendAttachmentState[0].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	mPipelineColorBlendAttachmentState[0].alphaBlendOp = vk::BlendOp::eAdd;
	mPipelineColorBlendAttachmentState[0].srcAlphaBlendFactor = vk::BlendFactor::eOne;
	mPipelineColorBlendAttachmentState[0].dstAlphaBlendFactor = vk::BlendFactor::eZero;

	mPipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	mPipelineColorBlendStateCreateInfo.logicOp = vk::LogicOp::eNoOp;
	mPipelineColorBlendStateCreateInfo.attachmentCount = 1;
	mPipelineColorBlendStateCreateInfo.pAttachments = mPipelineColorBlendAttachmentState;
	mPipelineColorBlendStateCreateInfo.blendConstants[0] = 1.0f;
	mPipelineColorBlendStateCreateInfo.blendConstants[1] = 1.0f;
	mPipelineColorBlendStateCreateInfo.blendConstants[2] = 1.0f;
	mPipelineColorBlendStateCreateInfo.blendConstants[3] = 1.0f;

	mPipelineViewportStateCreateInfo.viewportCount = 1;
	mPipelineViewportStateCreateInfo.scissorCount = 1;

	mPipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	mPipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	mPipelineDepthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
	mPipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	mPipelineDepthStencilStateCreateInfo.back.failOp = vk::StencilOp::eKeep;
	mPipelineDepthStencilStateCreateInfo.back.passOp = vk::StencilOp::eKeep;
	mPipelineDepthStencilStateCreateInfo.back.compareOp = vk::CompareOp::eAlways;
	mPipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	mPipelineDepthStencilStateCreateInfo.front = mPipelineDepthStencilStateCreateInfo.back;
	mPipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
	mPipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

	mPipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
	mPipelineMultisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

	mDescriptorSetLayoutCreateInfo.bindingCount = 1;
	mDescriptorSetLayoutCreateInfo.pBindings = mDescriptorSetLayoutBinding;
	mDescriptorSetLayoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;

	mDescriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	mDescriptorSetAllocateInfo.descriptorSetCount = 1;
	//mDescriptorSetAllocateInfo.pSetLayouts = &mDescriptorSetLayout;

	mPipelineLayoutCreateInfo.setLayoutCount = 1;

	mPipelineShaderStageCreateInfo[0].stage = vk::ShaderStageFlagBits::eVertex;
	mPipelineShaderStageCreateInfo[0].module = mVertShaderModule_XYZ_DIFFUSE;
	mPipelineShaderStageCreateInfo[0].pName = "main";
	mPipelineShaderStageCreateInfo[0].pSpecializationInfo = &mVertexSpecializationInfo;

	mPipelineShaderStageCreateInfo[1].stage = vk::ShaderStageFlagBits::eFragment;
	mPipelineShaderStageCreateInfo[1].module = mFragShaderModule_XYZ_DIFFUSE;
	mPipelineShaderStageCreateInfo[1].pName = "main";
	mPipelineShaderStageCreateInfo[1].pSpecializationInfo = &mPixelSpecializationInfo;

	mPipelineShaderStageCreateInfo[2].stage = vk::ShaderStageFlagBits::eGeometry;
	mPipelineShaderStageCreateInfo[2].module = mGeomShaderModule_XYZ_DIFFUSE;
	mPipelineShaderStageCreateInfo[2].pName = "main";
	mPipelineShaderStageCreateInfo[2].pSpecializationInfo = &mVertexSpecializationInfo;

	mGraphicsPipelineCreateInfo.pVertexInputState = &mPipelineVertexInputStateCreateInfo;
	mGraphicsPipelineCreateInfo.pInputAssemblyState = &mPipelineInputAssemblyStateCreateInfo;
	mGraphicsPipelineCreateInfo.pRasterizationState = &mPipelineRasterizationStateCreateInfo;
	mGraphicsPipelineCreateInfo.pColorBlendState = &mPipelineColorBlendStateCreateInfo;
	mGraphicsPipelineCreateInfo.pDepthStencilState = &mPipelineDepthStencilStateCreateInfo;
	mGraphicsPipelineCreateInfo.pViewportState = &mPipelineViewportStateCreateInfo;
	mGraphicsPipelineCreateInfo.pMultisampleState = &mPipelineMultisampleStateCreateInfo;
	mGraphicsPipelineCreateInfo.pStages = mPipelineShaderStageCreateInfo;
	//mGraphicsPipelineCreateInfo.renderPass = mStoreRenderPass;
	mGraphicsPipelineCreateInfo.pDynamicState = &mPipelineDynamicStateCreateInfo;
	mGraphicsPipelineCreateInfo.stageCount = 2;

	result = mDevice.createPipelineCache(&mPipelineCacheCreateInfo, nullptr, &mPipelineCache);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreatePipelineCache failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::SamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.magFilter = vk::Filter::eNearest;
	samplerCreateInfo.minFilter = vk::Filter::eNearest;
	samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
	samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 1;
	samplerCreateInfo.compareOp = vk::CompareOp::eNever;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	//samplerCreateInfo.compareEnable = true;

	result = mDevice.createSampler(&samplerCreateInfo, nullptr, &mSampler);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateSampler failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	/*
	Setup the texture to be written into the descriptor set.
	*/
	const vk::Format textureFormat = vk::Format::eB8G8R8A8Unorm;
	vk::FormatProperties formatProperties;
	const uint32_t textureColors[2] = { 0xffff0000, 0xff00ff00 };
	//mFormat = vk::Format::eB8G8R8A8Unorm;

	mPhysicalDevice.getFormatProperties(textureFormat, &formatProperties);

	vk::ImageCreateInfo imageCreateInfo2;
	imageCreateInfo2.imageType = vk::ImageType::e2D;
	imageCreateInfo2.format = textureFormat;
	imageCreateInfo2.extent = vk::Extent3D((uint32_t)width, (uint32_t)height, 1);
	imageCreateInfo2.mipLevels = 1;
	imageCreateInfo2.arrayLayers = 1;
	imageCreateInfo2.samples = vk::SampleCountFlagBits::e1;
	imageCreateInfo2.tiling = vk::ImageTiling::eLinear;
	imageCreateInfo2.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	//imageCreateInfo2.flags = 0;
	imageCreateInfo2.initialLayout = vk::ImageLayout::ePreinitialized;

	vk::MemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.allocationSize = 0;
	memoryAllocateInfo.memoryTypeIndex = 0;

	vk::MemoryRequirements memoryRequirements2;

	result = mDevice.createImage(&imageCreateInfo2, nullptr, &mImage);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateImage failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mDevice.getImageMemoryRequirements(mImage, &memoryRequirements2);
	memoryAllocateInfo.allocationSize = memoryRequirements2.size;
	GetMemoryTypeFromProperties(mPhysicalDeviceMemoryProperties, memoryRequirements2.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible, &memoryAllocateInfo.memoryTypeIndex);

	result = mDevice.allocateMemory(&memoryAllocateInfo, nullptr, &mDeviceMemory);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkAllocateMemory failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mDevice.bindImageMemory(mImage, mDeviceMemory, 0);

	vk::ImageSubresource imageSubresource;
	imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageSubresource.mipLevel = 0;
	imageSubresource.arrayLayer = 0;

	vk::SubresourceLayout subresourceLayout;
	void* data = nullptr;
	int32_t x = 0;
	int32_t y = 0;

	mDevice.getImageSubresourceLayout(mImage, &imageSubresource, &subresourceLayout);

	data = mDevice.mapMemory(mDeviceMemory, 0, memoryAllocateInfo.allocationSize, vk::MemoryMapFlags()).value;
	if (data == nullptr)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkMapMemory failed.";
		return;
	}

	for (y = 0; y < height; y++)
	{
		uint32_t *row = (uint32_t *)((char *)data + subresourceLayout.rowPitch * y);
		for (x = 0; x < width; x++)
		{
			row[x] = textureColors[(x & 1) ^ (y & 1)];
		}
	}

	mDevice.unmapMemory(mDeviceMemory);
	mImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	SetImageLayout(mImage, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::ImageViewCreateInfo imageViewCreateInfo2;
	imageViewCreateInfo2.image = mImage;
	imageViewCreateInfo2.viewType = vk::ImageViewType::e2D;
	imageViewCreateInfo2.format = textureFormat;
	imageViewCreateInfo2.components =
	{
		VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
	};
	imageViewCreateInfo2.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//imageViewCreateInfo.flags = 0;

	result = mDevice.createImageView(&imageViewCreateInfo2, nullptr, &mImageView);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::RealDevice vkCreateImageView failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	//mWriteDescriptorSet[0].dstSet = descriptorSet;
	mWriteDescriptorSet[0].dstBinding = 0;
	mWriteDescriptorSet[0].dstArrayElement = 0;
	mWriteDescriptorSet[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	mWriteDescriptorSet[0].descriptorCount = 1;
	mWriteDescriptorSet[0].pBufferInfo = &mDescriptorBufferInfo[0];

	//mWriteDescriptorSet[1].dstSet = descriptorSet;
	mWriteDescriptorSet[1].dstBinding = 1;
	mWriteDescriptorSet[1].dstArrayElement = 0;
	mWriteDescriptorSet[1].descriptorType = vk::DescriptorType::eUniformBuffer;
	mWriteDescriptorSet[1].descriptorCount = 1;
	mWriteDescriptorSet[1].pBufferInfo = &mDescriptorBufferInfo[1];

	//mWriteDescriptorSet[2].dstSet = descriptorSet;
	mWriteDescriptorSet[2].dstBinding = 2;
	mWriteDescriptorSet[2].dstArrayElement = 0;
	mWriteDescriptorSet[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	mWriteDescriptorSet[2].descriptorCount = 1;
	mWriteDescriptorSet[2].pImageInfo = mDeviceState.mDescriptorImageInfo;

	mCommandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
	mCommandBufferAllocateInfo.commandPool = mCommandPool;
	mCommandBufferAllocateInfo.commandBufferCount = 1;

	mDevice.allocateCommandBuffers(&mCommandBufferAllocateInfo, &mCommandBuffer);

	mBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	mSubmitInfo.commandBufferCount = 1;
	mSubmitInfo.pCommandBuffers = &mCommandBuffer;

	//Set some device state stuff.
	for (size_t i = 0; i < 16; i++)
	{
		mDeviceState.mDescriptorImageInfo[i].sampler = mSampler;
		mDeviceState.mDescriptorImageInfo[i].imageView = mImageView;
		mDeviceState.mDescriptorImageInfo[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	}

	//initialize vulkan/d3d9 viewport and scissor structures.
	//mDeviceState.mViewport.y = (float)mPresentationParameters.BackBufferHeight;
	mDeviceState.mViewport.width = (float)width;
	//mDeviceState.mViewport.height = -(float)mPresentationParameters.BackBufferHeight;
	mDeviceState.mViewport.height = (float)height;
	mDeviceState.mViewport.minDepth = 0.0f;
	mDeviceState.mViewport.maxDepth = 1.0f;

	mDeviceState.m9Viewport.Width = (DWORD)mDeviceState.mViewport.width;
	mDeviceState.m9Viewport.Height = (DWORD)mDeviceState.mViewport.height;
	mDeviceState.m9Viewport.MinZ = mDeviceState.mViewport.minDepth;
	mDeviceState.m9Viewport.MaxZ = mDeviceState.mViewport.maxDepth;

	mDeviceState.mScissor.offset.x = 0;
	mDeviceState.mScissor.offset.y = 0;
	mDeviceState.mScissor.extent.width = width;
	mDeviceState.mScissor.extent.height = height;

	mDeviceState.m9Scissor.right = width;
	mDeviceState.m9Scissor.bottom = height;
	mDeviceState.m9Scissor.left = 0;
	mDeviceState.m9Scissor.top = 0;

	Light light = {};
	mDeviceState.mLights.push_back(light);

	mBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	//revisit - light should be sized dynamically. Really more that 4 lights is stupid but this limit isn't correct behavior.
	CreateBuffer(sizeof(Light) * 4, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, mLightBuffer, mLightBufferMemory);
	CreateBuffer(sizeof(D3DMATERIAL9), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, mMaterialBuffer, mMaterialBufferMemory);
}

RealDevice::~RealDevice()
{
	BOOST_LOG_TRIVIAL(info) << "RealDevice::~RealDevice";
	delete[] mQueueFamilyProperties;
	if (mDevice == vk::Device())
	{
		return;
	}

	mDrawBuffer.clear();
	mSamplerRequests.clear();

	mDeviceState.mRenderTarget.reset();
	
	mDevice.destroyBuffer(mLightBuffer, nullptr);
	mDevice.freeMemory(mLightBufferMemory, nullptr);
	mDevice.destroyBuffer(mMaterialBuffer, nullptr);
	mDevice.freeMemory(mMaterialBufferMemory, nullptr);
	mDevice.destroyImageView(mImageView, nullptr);
	mDevice.destroyImage(mImage, nullptr);
	mDevice.freeMemory(mDeviceMemory, nullptr);
	mDevice.destroySampler(mSampler, nullptr);

	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mGeomShaderModule_XYZ_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW_TEX1, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_TEX1, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_TEX1, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW_TEX2, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_TEX2, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_TEX2, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW_DIFFUSE_TEX1, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_DIFFUSE_TEX1, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_DIFFUSE_TEX1, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZRHW_DIFFUSE_TEX2, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_DIFFUSE_TEX2, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_DIFFUSE_TEX2, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_NORMAL, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_NORMAL, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_NORMAL_TEX1, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_NORMAL_TEX1, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_NORMAL_TEX2, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_NORMAL_TEX2, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_NORMAL_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_NORMAL_DIFFUSE, nullptr);
	mDevice.destroyShaderModule(mVertShaderModule_XYZ_NORMAL_DIFFUSE_TEX2, nullptr);
	mDevice.destroyShaderModule(mFragShaderModule_XYZ_NORMAL_DIFFUSE_TEX2, nullptr);
	mDevice.destroyPipelineCache(mPipelineCache, nullptr);

	mDevice.freeCommandBuffers(mCommandPool, 1, &mCommandBuffer);
	mDevice.freeCommandBuffers(mCommandPool, 2, mCommandBuffers);
	mDevice.destroyCommandPool(mCommandPool, nullptr);

	mDevice.destroyDescriptorPool(mDescriptorPool, nullptr);
	//destroy render targets before the device
	for(int i=0; i < mRenderTargets.size();i++)
	{
		mRenderTargets[i].reset();
	}
	mRenderTargets.clear();


	mDevice.destroy();
}

void RealDevice::SetImageLayout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout, uint32_t levelCount, uint32_t mipIndex, uint32_t layerCount)
{
	/*
	This is just a helper method to reduce repeat code.
	*/
	vk::Result result;
	vk::CommandBuffer commandBuffer;

	if (aspectMask == vk::ImageAspectFlags())
	{
		aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	vk::CommandBufferAllocateInfo commandBufferInfo = {};
	commandBufferInfo.commandPool = mCommandPool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = 1;

	result = mDevice.allocateCommandBuffers(&commandBufferInfo, &commandBuffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::SetImageLayout vkAllocateCommandBuffers failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo;
	commandBufferInheritanceInfo.subpass = 0;
	commandBufferInheritanceInfo.occlusionQueryEnable = VK_FALSE;

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

	result = commandBuffer.begin(&commandBufferBeginInfo);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::SetImageLayout vkBeginCommandBuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	ReallySetImageLayout(commandBuffer, image, aspectMask, oldImageLayout, newImageLayout, levelCount, mipIndex, layerCount);

	commandBuffer.end();

	vk::CommandBuffer commandBuffers[] = { commandBuffer };
	vk::Fence nullFence;
	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	submitInfo.signalSemaphoreCount = 0;

	result = mQueue.submit(1, &submitInfo, nullFence);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::SetImageLayout vkQueueSubmit failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mQueue.waitIdle();
	mDevice.freeCommandBuffers(mCommandPool, 1, commandBuffers);
}

void RealDevice::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlagBits properties, vk::Buffer& buffer, vk::DeviceMemory& deviceMemory)
{
	vk::Result result; // = VK_SUCCESS

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	result = mDevice.createBuffer(&bufferInfo, nullptr, &buffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::CreateBuffer vkCreateBuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::MemoryRequirements memoryRequirements;
	mDevice.getBufferMemoryRequirements(buffer, &memoryRequirements);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memoryRequirements.size;
	//allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (!GetMemoryTypeFromProperties(mPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, properties, &allocInfo.memoryTypeIndex))
	{
		BOOST_LOG_TRIVIAL(fatal) << "Memory type index not found!";
		return;
	}

	result = mDevice.allocateMemory(&allocInfo, nullptr, &deviceMemory);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealDevice::CreateBuffer vkCreateBuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mDevice.bindBufferMemory(buffer, deviceMemory, 0);
}

void RealDevice::CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
	mCommandBuffer.begin(&mBeginInfo);
	{
		mCopyRegion.size = size;
		mCommandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &mCopyRegion);
	}
	mCommandBuffer.end();
	mQueue.submit(1, &mSubmitInfo, nullptr);
	mQueue.waitIdle();
	mCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources); //So far resetting a command buffer is about 10 times faster than allocating a new one.
}