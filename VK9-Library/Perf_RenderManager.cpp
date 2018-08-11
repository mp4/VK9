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

/*
"Whoever pursues righteousness and kindness will find life, righteousness, and honor." (Proverbs 21:21, ESV)
*/

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include "Perf_RenderManager.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/format.hpp>

#include "Utilities.h"
#include "CTypes.h"

#include "CCubeTexture9.h"
#include "CVolumeTexture9.h"
#include "CBaseTexture9.h"
#include "CTexture9.h"
#include "CIndexBuffer9.h"
#include "CVertexBuffer9.h"
#include "CVertexDeclaration9.h"
#include "CPixelShader9.h"
#include "CVertexShader9.h"

RenderManager::RenderManager()
{

}

RenderManager::~RenderManager()
{
}

void RenderManager::UpdateBuffer(std::shared_ptr<RealDevice> realDevice)
{ //Vulkan doesn't allow vkCmdUpdateBuffer inside of a render pass.

	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	if (!realDevice->mDeviceState.mRenderTarget->mIsSceneStarted)
	{
		this->StartScene(realDevice, false);
	}

	//The dirty flag for lights can be set by enable light or set light.
	if (deviceState.mAreLightsDirty)
	{
		currentBuffer.updateBuffer(realDevice->mLightBuffer, 0, sizeof(Light)*deviceState.mLights.size(), deviceState.mLights.data()); //context->mSpecializationConstants.lightCount
		deviceState.mAreLightsDirty = false;
	}

	if (deviceState.mIsMaterialDirty)
	{
		currentBuffer.updateBuffer(realDevice->mMaterialBuffer, 0, sizeof(D3DMATERIAL9), &deviceState.mMaterial);
		deviceState.mIsMaterialDirty = false;
	}
}

void RenderManager::StartScene(std::shared_ptr<RealDevice> realDevice, bool clear)
{
	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	realDevice->mDeviceState.mRenderTarget->StartScene(currentBuffer, deviceState, clear, deviceState.hasPresented);
	deviceState.hasPresented = false;
}

void RenderManager::StopScene(std::shared_ptr<RealDevice> realDevice)
{
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	realDevice->mDeviceState.mRenderTarget->StopScene(currentBuffer, realDevice->mQueue);
}

void RenderManager::CopyImage(std::shared_ptr<RealDevice> realDevice, vk::Image srcImage, vk::Image dstImage, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t depth, uint32_t srcMip, uint32_t dstMip)
{
	vk::Result result;
	vk::CommandBuffer commandBuffer;
	auto& device = realDevice->mDevice;

	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandPool = realDevice->mCommandPool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = 1;

	result = device.allocateCommandBuffers(&commandBufferInfo, &commandBuffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CopyImage vkAllocateCommandBuffers failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo;
	commandBufferInheritanceInfo.renderPass = nullptr;
	commandBufferInheritanceInfo.subpass = 0;
	commandBufferInheritanceInfo.framebuffer = nullptr;
	commandBufferInheritanceInfo.occlusionQueryEnable = VK_FALSE;
	//commandBufferInheritanceInfo.queryFlags = 0;
	//commandBufferInheritanceInfo.pipelineStatistics = 0;

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	//commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

	result = commandBuffer.begin(&commandBufferBeginInfo);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CopyImage vkBeginCommandBuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	ReallyCopyImage(commandBuffer, srcImage, dstImage, x, y, width, height, depth, srcMip, dstMip, 0, 0);

	commandBuffer.end();

	vk::CommandBuffer commandBuffers[] = { commandBuffer };
	vk::Fence nullFence;
	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	result = realDevice->mQueue.submit(1, &submitInfo, nullFence);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CopyImage vkQueueSubmit failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mQueue.waitIdle();
	device.freeCommandBuffers(realDevice->mCommandPool, 1, commandBuffers);
}

void RenderManager::Clear(std::shared_ptr<RealDevice> realDevice, DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];
	auto& deviceState = realDevice->mDeviceState;

	realDevice->mDeviceState.mRenderTarget->Clear(currentBuffer, deviceState, Count, pRects, Flags, Color, Z, Stencil);
}

void RenderManager::Present(std::shared_ptr<RealDevice> realDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
	if (!realDevice->mDeviceState.mRenderTarget->mIsSceneStarted)
	{
		this->StartScene(realDevice, false);
	}
	this->StopScene(realDevice);

	//vk::Result result;
	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];
	auto swapchain = mStateManager.GetSwapChain(realDevice, hDestWindowOverride);

	swapchain->Present(currentBuffer, realDevice->mQueue, deviceState.mRenderTarget->mColorSurface->mStagingImage);
	deviceState.hasPresented = true;
	realDevice->mCurrentCommandBuffer = !realDevice->mCurrentCommandBuffer;

	//Clean up pipes.
	FlushDrawBufffer(realDevice);

	//Clean up unreferenced resources.
	//mGarbageManager.DestroyHandles();

	//Print(mDeviceState.mTransforms);
}

void RenderManager::DrawIndexedPrimitive(std::shared_ptr<RealDevice> realDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	if (deviceState.mIndexBuffer == nullptr)
	{
		BOOST_LOG_TRIVIAL(warning) << "CDevice9::DrawIndexedPrimitive called with null index buffer.";
		return;
	}

	if (!realDevice->mDeviceState.mRenderTarget->mIsSceneStarted)
	{
		this->StartScene(realDevice, false);
	}

	std::shared_ptr<DrawContext> context = std::make_shared<DrawContext>(realDevice.get());
	std::shared_ptr<ResourceContext> resourceContext = std::make_shared<ResourceContext>(realDevice.get());

	BeginDraw(realDevice, context, resourceContext, Type);

	/*
	https://msdn.microsoft.com/en-us/library/windows/desktop/bb174369(v=vs.85).aspx
	https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDrawIndexed.html
	*/
	currentBuffer.drawIndexed(std::min(deviceState.mIndexBuffer->mSize, ConvertPrimitiveCountToVertexCount(Type, PrimitiveCount)), 1, StartIndex, BaseVertexIndex, 0);
}

void RenderManager::DrawPrimitive(std::shared_ptr<RealDevice> realDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	if (!realDevice->mDeviceState.mRenderTarget->mIsSceneStarted)
	{
		this->StartScene(realDevice, false);
	}

	std::shared_ptr<DrawContext> context = std::make_shared<DrawContext>(realDevice.get());
	std::shared_ptr<ResourceContext> resourceContext = std::make_shared<ResourceContext>(realDevice.get());

	BeginDraw(realDevice, context, resourceContext, PrimitiveType);

	currentBuffer.draw(std::min(realDevice->mVertexCount, ConvertPrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount)), 1, StartVertex, 0);
}

void RenderManager::UpdateTexture(std::shared_ptr<RealDevice> realDevice, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
	if (pSourceTexture == nullptr || pDestinationTexture == nullptr)
	{
		return;
	}

	auto& device = realDevice->mDevice;

	vk::CommandBuffer commandBuffer;
	vk::Result result;

	vk::CommandBufferAllocateInfo commandBufferInfo = {};
	commandBufferInfo.commandPool = realDevice->mCommandPool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = 1;

	result = device.allocateCommandBuffers(&commandBufferInfo, &commandBuffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::UpdateTexture vkAllocateCommandBuffers failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo;
	commandBufferInheritanceInfo.renderPass = nullptr;
	commandBufferInheritanceInfo.subpass = 0;
	commandBufferInheritanceInfo.framebuffer = nullptr;
	commandBufferInheritanceInfo.occlusionQueryEnable = VK_FALSE;
	//commandBufferInheritanceInfo.queryFlags = 0;
	//commandBufferInheritanceInfo.pipelineStatistics = 0;

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	//commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

	result = commandBuffer.begin(&commandBufferBeginInfo);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::UpdateTexture vkBeginCommandBuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	//TODO: Handle dirty regions and multiple mip levels.

	std::shared_ptr<RealTexture> source;
	std::shared_ptr<RealTexture> target;
	uint32_t width = 0;
	uint32_t height = 0;

	if (pDestinationTexture->GetType() != D3DRTYPE_CUBETEXTURE)
	{
		CTexture9& target9 = (*(CTexture9*)pDestinationTexture);
		target = mStateManager.mTextures[target9.mId];

		ReallySetImageLayout(commandBuffer, target->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);
	}
	else
	{
		CCubeTexture9& target9 = (*(CCubeTexture9*)pDestinationTexture);
		target = mStateManager.mTextures[target9.mId];

		ReallySetImageLayout(commandBuffer, target->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 6);
	}

	if (pSourceTexture->GetType() == D3DRTYPE_CUBETEXTURE)
	{
		CCubeTexture9& source9 = (*(CCubeTexture9*)pSourceTexture);
		source = mStateManager.mTextures[source9.mId];

		ReallySetImageLayout(commandBuffer, source->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 6);
		ReallyCopyImage(commandBuffer, source->mImage, target->mImage, 0, 0, source9.mEdgeLength, source9.mEdgeLength, 1, 0, 0, 0, 0);
	}
	else if (pSourceTexture->GetType() == D3DRTYPE_VOLUMETEXTURE)
	{
		CVolumeTexture9& source9 = (*(CVolumeTexture9*)pSourceTexture);
		source = mStateManager.mTextures[source9.mId];

		ReallySetImageLayout(commandBuffer, source->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);
		ReallyCopyImage(commandBuffer, source->mImage, target->mImage, 0, 0, source9.mWidth, source9.mHeight, source9.mDepth, 0, 0, 0, 0);
	}
	else
	{
		CTexture9& source9 = (*(CTexture9*)pSourceTexture);
		source = mStateManager.mTextures[source9.mId];

		ReallySetImageLayout(commandBuffer, source->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);
		ReallyCopyImage(commandBuffer, source->mImage, target->mImage, 0, 0, source9.mWidth, source9.mHeight, 1, 0, 0, 0, 0);
	}

	if (pDestinationTexture->GetType() != D3DRTYPE_CUBETEXTURE)
	{
		ReallySetImageLayout(commandBuffer, target->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, 1, 0, 1);
	}
	else
	{
		ReallySetImageLayout(commandBuffer, target->mImage, vk::ImageAspectFlags(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, 1, 0, 6);
	}

	commandBuffer.end();

	vk::CommandBuffer commandBuffers[] = { commandBuffer };
	vk::Fence nullFence;

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;

	result = realDevice->mQueue.submit(1, &submitInfo, nullFence);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::UpdateTexture vkQueueSubmit failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mQueue.waitIdle();
	device.freeCommandBuffers(realDevice->mCommandPool, 1, commandBuffers);
}

void RenderManager::BeginDraw(std::shared_ptr<RealDevice> realDevice, std::shared_ptr<DrawContext> context, std::shared_ptr<ResourceContext> resourceContext, D3DPRIMITIVETYPE type)
{
	VkResult result = VK_SUCCESS;
	boost::container::flat_map<D3DRENDERSTATETYPE, DWORD>::const_iterator searchResult;

	auto& device = realDevice->mDevice;
	auto& deviceState = realDevice->mDeviceState;
	auto& currentBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];

	/**********************************************
	* Update the stuff that need to be done outside of a render pass.
	**********************************************/
	if (deviceState.mAreLightsDirty || deviceState.mIsMaterialDirty)
	{
		currentBuffer.endRenderPass();
		UpdateBuffer(realDevice);
		currentBuffer.beginRenderPass(&realDevice->mDeviceState.mRenderTarget->mRenderPassBeginInfo, vk::SubpassContents::eInline);
	}

	/**********************************************
	* Update the textures that are currently mapped.
	**********************************************/
	auto& samplerStates = deviceState.mSamplerStates;

	for (size_t i = 0; i < 16; i++)
	{
		vk::DescriptorImageInfo& targetSampler = deviceState.mDescriptorImageInfo[i];

		if (deviceState.mTextures[i] != nullptr)
		{
			std::shared_ptr<SamplerRequest> request = std::make_shared<SamplerRequest>(realDevice.get());
			auto& currentSampler = samplerStates[request->SamplerIndex];

			if (deviceState.mTextures[i]->GetType() == D3DRTYPE_CUBETEXTURE)
			{
				CCubeTexture9* texture9 = (CCubeTexture9*)deviceState.mTextures[i];
				auto& texture = mStateManager.mTextures[texture9->mId];

				request->MaxLod = texture9->mLevels;
				targetSampler.imageView = texture->mImageView;
			}
			else
			{
				CTexture9* texture9 = (CTexture9*)deviceState.mTextures[i];
				auto& texture = mStateManager.mTextures[texture9->mId];

				request->MaxLod = texture9->mLevels;
				targetSampler.imageView = texture->mImageView;
			}

			request->MagFilter = (D3DTEXTUREFILTERTYPE)currentSampler[D3DSAMP_MAGFILTER];
			request->MinFilter = (D3DTEXTUREFILTERTYPE)currentSampler[D3DSAMP_MINFILTER];
			request->AddressModeU = (D3DTEXTUREADDRESS)currentSampler[D3DSAMP_ADDRESSU];
			request->AddressModeV = (D3DTEXTUREADDRESS)currentSampler[D3DSAMP_ADDRESSV];
			request->AddressModeW = (D3DTEXTUREADDRESS)currentSampler[D3DSAMP_ADDRESSW];
			request->MaxAnisotropy = currentSampler[D3DSAMP_MAXANISOTROPY];
			request->MipmapMode = (D3DTEXTUREFILTERTYPE)currentSampler[D3DSAMP_MIPFILTER];
			request->MipLodBias = currentSampler[D3DSAMP_MIPMAPLODBIAS]; //bit_cast();


			for (size_t i = 0; i < realDevice->mSamplerRequests.size(); i++)
			{
				auto& storedRequest = realDevice->mSamplerRequests[i];
				if (request->MagFilter == storedRequest->MagFilter
					&& request->MinFilter == storedRequest->MinFilter
					&& request->AddressModeU == storedRequest->AddressModeU
					&& request->AddressModeV == storedRequest->AddressModeV
					&& request->AddressModeW == storedRequest->AddressModeW
					&& request->MaxAnisotropy == storedRequest->MaxAnisotropy
					&& request->MipmapMode == storedRequest->MipmapMode
					&& request->MipLodBias == storedRequest->MipLodBias
					&& request->MaxLod == storedRequest->MaxLod)
				{
					request->Sampler = storedRequest->Sampler;
					request->mRealDevice = nullptr; //Not owner.
					storedRequest->LastUsed = std::chrono::steady_clock::now();
				}
			}

			if (request->Sampler == vk::Sampler())
			{
				CreateSampler(realDevice, request);
			}

			targetSampler.sampler = request->Sampler;
			targetSampler.imageLayout = vk::ImageLayout::eGeneral;
		}
		else
		{
			targetSampler.sampler = realDevice->mSampler;
			targetSampler.imageView = realDevice->mImageView;
			targetSampler.imageLayout = vk::ImageLayout::eGeneral;
		}

	}

	/**********************************************
	* Setup context.
	**********************************************/
	context->PrimitiveType = type;

	if (deviceState.mHasVertexDeclaration)
	{
		context->VertexDeclaration = deviceState.mVertexDeclaration;
	}
	else if (deviceState.mHasFVF)
	{
		context->FVF = deviceState.mFVF;
	}

	//TODO: revisit if it's valid to have declaration or FVF with either shader type.

	if (deviceState.mHasVertexShader)
	{
		context->VertexShader = deviceState.mVertexShader; //vert	
	}

	if (deviceState.mHasPixelShader)
	{
		context->PixelShader = deviceState.mPixelShader; //pixel		
	}

	if (deviceState.mVertexShader != nullptr)
	{
		context->mVertexShaderConstantSlots = deviceState.mVertexShaderConstantSlots;
		resourceContext->WasShader = true;
	}

	if (deviceState.mPixelShader != nullptr)
	{
		context->mPixelShaderConstantSlots = deviceState.mPixelShaderConstantSlots;
	}

	context->StreamCount = deviceState.mStreamSources.size();
	context->mSpecializationConstants = deviceState.mSpecializationConstants;

	SpecializationConstants& constants = context->mSpecializationConstants;
	ShaderConstantSlots& vertexSlots = context->mVertexShaderConstantSlots;
	ShaderConstantSlots& pixelSlots = context->mPixelShaderConstantSlots;

	constants.lightCount = deviceState.mLights.size();
	constants.textureCount = 0;

	for (size_t i = 0; i < 16; i++)
	{
		if (deviceState.mTextures[i] != nullptr)
		{
			constants.textureCount++;
		}
		else
		{
			break;
		}
	}

	deviceState.mSpecializationConstants.lightCount = constants.lightCount;
	deviceState.mSpecializationConstants.textureCount = constants.textureCount;

	int i = 0;
	BOOST_FOREACH(auto& source, deviceState.mStreamSources)
	{
		realDevice->mVertexInputBindingDescription[i].binding = source.first;
		realDevice->mVertexInputBindingDescription[i].stride = source.second.Stride;
		realDevice->mVertexInputBindingDescription[i].inputRate = vk::VertexInputRate::eVertex;

		context->Bindings[source.first] = source.second.Stride;

		i++;
	}

	/**********************************************
	* Check for existing pipeline. Create one if there isn't a matching one.
	**********************************************/

	for (size_t i = 0; i < realDevice->mDrawBuffer.size(); i++)
	{
		auto& drawBuffer = (*realDevice->mDrawBuffer[i]);

		if (drawBuffer.PrimitiveType == context->PrimitiveType
			&& drawBuffer.StreamCount == context->StreamCount

			&& drawBuffer.VertexShader == context->VertexShader
			&& drawBuffer.PixelShader == context->PixelShader

			&& drawBuffer.FVF == context->FVF
			&& drawBuffer.VertexDeclaration == context->VertexDeclaration

			&& !memcmp(&drawBuffer.mSpecializationConstants, &constants, sizeof(SpecializationConstants))
			&& !memcmp(&drawBuffer.mVertexShaderConstantSlots, &vertexSlots, sizeof(ShaderConstantSlots))
			&& !memcmp(&drawBuffer.mPixelShaderConstantSlots, &pixelSlots, sizeof(ShaderConstantSlots))
			)
		{
			if (!memcmp(&drawBuffer.Bindings, &context->Bindings, 64 * sizeof(UINT)))
			{
				context->Pipeline = drawBuffer.Pipeline;
				context->PipelineLayout = drawBuffer.PipelineLayout;
				context->DescriptorSetLayout = drawBuffer.DescriptorSetLayout;
				context->mRealDevice = nullptr; //Not owner.
				drawBuffer.LastUsed = std::chrono::steady_clock::now();
				break;
			}
		}
	}

	if (context->Pipeline == vk::Pipeline())
	{
		CreatePipe(realDevice, context); //If we didn't find a matching pipeline then create a new one.	
	}

	/*
	https://msdn.microsoft.com/en-us/library/windows/desktop/bb205599(v=vs.85).aspx
	The units for the D3DRS_DEPTHBIAS and D3DRS_SLOPESCALEDEPTHBIAS render states depend on whether z-buffering or w-buffering is enabled.
	The bias is not applied to any line and point primitive.
	*/
	if (constants.zEnable != D3DZB_FALSE && type > 3)
	{
		currentBuffer.setDepthBias(constants.depthBias, 0.0f, constants.slopeScaleDepthBias);
	}
	else
	{
		currentBuffer.setDepthBias(0.0f, 0.0f, 0.0f);
	}

	/**********************************************
	* Update transformation structure.
	**********************************************/
	if (context->VertexShader == nullptr)
	{
		UpdatePushConstants(realDevice, context);
	}
	else
	{
		currentBuffer.pushConstants(context->PipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, UBO_SIZE * 2, &deviceState.mPushConstants);
	}

	/**********************************************
	* Check for existing DescriptorSet. Create one if there isn't a matching one.
	**********************************************/

	if (context->DescriptorSetLayout != vk::DescriptorSetLayout())
	{
		std::copy(std::begin(deviceState.mDescriptorImageInfo), std::end(deviceState.mDescriptorImageInfo), std::begin(resourceContext->DescriptorImageInfo));

		if (context->VertexShader == nullptr)
		{
			realDevice->mDescriptorBufferInfo[0].buffer = realDevice->mLightBuffer;
			realDevice->mDescriptorBufferInfo[0].offset = 0;
			realDevice->mDescriptorBufferInfo[0].range = sizeof(Light) * deviceState.mLights.size(); //4; 

			realDevice->mDescriptorBufferInfo[1].buffer = realDevice->mMaterialBuffer;
			realDevice->mDescriptorBufferInfo[1].offset = 0;
			realDevice->mDescriptorBufferInfo[1].range = sizeof(D3DMATERIAL9);

			realDevice->mWriteDescriptorSet[0].descriptorType = vk::DescriptorType::eUniformBuffer;
			realDevice->mWriteDescriptorSet[0].dstSet = resourceContext->DescriptorSet;
			realDevice->mWriteDescriptorSet[0].descriptorCount = 1;
			realDevice->mWriteDescriptorSet[0].pBufferInfo = &realDevice->mDescriptorBufferInfo[0];

			realDevice->mWriteDescriptorSet[1].dstSet = resourceContext->DescriptorSet;
			realDevice->mWriteDescriptorSet[1].descriptorCount = 1;
			realDevice->mWriteDescriptorSet[1].pBufferInfo = &realDevice->mDescriptorBufferInfo[1];

			realDevice->mWriteDescriptorSet[2].dstSet = resourceContext->DescriptorSet;
			realDevice->mWriteDescriptorSet[2].descriptorCount = constants.textureCount;
			realDevice->mWriteDescriptorSet[2].pImageInfo = resourceContext->DescriptorImageInfo;

			if (constants.textureCount)
			{
				currentBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, context->PipelineLayout, 0, 3, realDevice->mWriteDescriptorSet);
			}
			else
			{
				currentBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, context->PipelineLayout, 0, 2, realDevice->mWriteDescriptorSet);
			}
		}
		else
		{
			realDevice->mWriteDescriptorSet[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
			realDevice->mWriteDescriptorSet[0].dstSet = resourceContext->DescriptorSet;
			realDevice->mWriteDescriptorSet[0].descriptorCount = constants.textureCount; //Revisit
			realDevice->mWriteDescriptorSet[0].pImageInfo = resourceContext->DescriptorImageInfo;

			currentBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, context->PipelineLayout, 0, 1, realDevice->mWriteDescriptorSet);
		}
	}

	/**********************************************
	* Setup bindings
	**********************************************/

	//TODO: I need to find a way to prevent binding on every draw call.

	//if (!mIsDirty || mLastVkPipeline != context->Pipeline)
	//{
	currentBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, context->Pipeline);
	//	mLastVkPipeline = context->Pipeline;
	//}

	realDevice->mVertexCount = 0;

	if (deviceState.mIndexBuffer != nullptr)
	{
		currentBuffer.bindIndexBuffer(deviceState.mIndexBuffer->mBuffer, 0, deviceState.mIndexBuffer->mIndexType);
	}

	BOOST_FOREACH(auto& source, deviceState.mStreamSources)
	{
		auto& buffer = mStateManager.mVertexBuffers[source.second.StreamData->mId];
		currentBuffer.bindVertexBuffers(source.first, 1, &buffer->mBuffer, &source.second.OffsetInBytes);
		realDevice->mVertexCount += source.second.StreamData->mSize;
	}

	realDevice->mIsDirty = false;
}

void RenderManager::CreatePipe(std::shared_ptr<RealDevice> realDevice, std::shared_ptr<DrawContext> context)
{
	vk::Result result;
	auto& deviceState = realDevice->mDeviceState;
	auto& device = realDevice->mDevice;

	/**********************************************
	* Figure out flags
	**********************************************/
	SpecializationConstants& constants = context->mSpecializationConstants;
	uint32_t attributeCount = 0;
	uint32_t textureCount = 0; //constants.textureCount
	uint32_t lightCount = constants.lightCount;
	uint32_t positionSize = 3;
	BOOL hasColor = 0;
	BOOL hasPosition = 0;
	BOOL hasNormal = 0;
	BOOL isTransformed = 0;
	BOOL isLightingEnabled = constants.lighting;

	if (context->VertexDeclaration != nullptr)
	{
		auto vertexDeclaration = context->VertexDeclaration;

		hasColor = vertexDeclaration->mHasColor;
		hasPosition = vertexDeclaration->mHasPosition;
		hasNormal = vertexDeclaration->mHasNormal;
		textureCount = vertexDeclaration->mTextureCount;
	}
	else if (context->FVF)
	{
		if ((context->FVF & D3DFVF_XYZRHW) == D3DFVF_XYZRHW)
		{
			positionSize = 4;
			hasPosition = true;
			isTransformed = true;
		}
		else
		{
			if ((context->FVF & D3DFVF_XYZW) == D3DFVF_XYZW)
			{
				positionSize = 4;
				hasPosition = true;
			}
			else if ((context->FVF & D3DFVF_XYZ) == D3DFVF_XYZ)
			{
				positionSize = 3;
				hasPosition = true;
			}

			if ((context->FVF & D3DFVF_NORMAL) == D3DFVF_NORMAL)
			{
				hasNormal = true;
			}
		}

		if ((context->FVF & D3DFVF_PSIZE) == D3DFVF_PSIZE)
		{
			BOOST_LOG_TRIVIAL(warning) << "RenderManager::CreatePipe D3DFVF_PSIZE is not implemented!";
		}

		if ((context->FVF & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE)
		{
			hasColor = true;
		}

		if ((context->FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR)
		{
			BOOST_LOG_TRIVIAL(warning) << "RenderManager::CreatePipe D3DFVF_SPECULAR is not implemented!";
		}

		textureCount = ConvertFormat(context->FVF);
	}
	else if (context->VertexShader != nullptr)
	{
		//Nothing so far.
	}
	else
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported layout definition.";
	}

	attributeCount += hasColor;
	attributeCount += hasPosition;
	attributeCount += hasNormal;
	attributeCount += textureCount;

	/**********************************************
	* Figure out render states & texture states
	**********************************************/
	realDevice->mPipelineColorBlendAttachmentState[0].colorWriteMask = (vk::ColorComponentFlagBits)constants.colorWriteEnable;
	realDevice->mPipelineColorBlendAttachmentState[0].blendEnable = constants.alphaBlendEnable;

	realDevice->mPipelineColorBlendAttachmentState[0].colorBlendOp = ConvertColorOperation(constants.blendOperation);
	realDevice->mPipelineColorBlendAttachmentState[0].srcColorBlendFactor = ConvertColorFactor(constants.sourceBlend);
	realDevice->mPipelineColorBlendAttachmentState[0].dstColorBlendFactor = ConvertColorFactor(constants.destinationBlend);

	realDevice->mPipelineColorBlendAttachmentState[0].alphaBlendOp = ConvertColorOperation(constants.blendOperationAlpha);
	realDevice->mPipelineColorBlendAttachmentState[0].srcAlphaBlendFactor = ConvertColorFactor(constants.sourceBlendAlpha);
	realDevice->mPipelineColorBlendAttachmentState[0].dstAlphaBlendFactor = ConvertColorFactor(constants.destinationBlendAlpha);

	SetCulling(realDevice->mPipelineRasterizationStateCreateInfo, (D3DCULL)constants.cullMode);
	realDevice->mPipelineRasterizationStateCreateInfo.polygonMode = ConvertFillMode((D3DFILLMODE)constants.fillMode);
	realDevice->mPipelineInputAssemblyStateCreateInfo.topology = ConvertPrimitiveType(context->PrimitiveType);

	realDevice->mPipelineDepthStencilStateCreateInfo.depthTestEnable = constants.zEnable; //= VK_TRUE;
	realDevice->mPipelineDepthStencilStateCreateInfo.depthWriteEnable = constants.zWriteEnable; //VK_TRUE;
	realDevice->mPipelineDepthStencilStateCreateInfo.depthCompareOp = ConvertCompareOperation(constants.zFunction);  //VK_COMPARE_OP_LESS_OR_EQUAL;
	//realDevice->mPipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = true; //= constants.bound;
	realDevice->mPipelineDepthStencilStateCreateInfo.stencilTestEnable = constants.stencilEnable; //VK_FALSE;

	//twoSidedStencilMode

	/*
	uint32_t stencilMask = 0xFFFFFFFF;
	uint32_t stencilWriteMask = 0xFFFFFFFF;
	*/

	/*
	compareMask( compareMask_ )
	, writeMask( writeMask_ )
	*/

	realDevice->mPipelineDepthStencilStateCreateInfo.back.reference = constants.stencilReference;
	realDevice->mPipelineDepthStencilStateCreateInfo.back.compareMask = constants.stencilMask;
	realDevice->mPipelineDepthStencilStateCreateInfo.back.writeMask = constants.stencilWriteMask;

	realDevice->mPipelineDepthStencilStateCreateInfo.front.reference = constants.stencilReference;
	realDevice->mPipelineDepthStencilStateCreateInfo.front.compareMask = constants.stencilMask;
	realDevice->mPipelineDepthStencilStateCreateInfo.front.writeMask = constants.stencilWriteMask;

	if (constants.cullMode == D3DCULL_CCW)
	{
		realDevice->mPipelineDepthStencilStateCreateInfo.back.failOp = ConvertStencilOperation(constants.ccwStencilFail);
		realDevice->mPipelineDepthStencilStateCreateInfo.back.passOp = ConvertStencilOperation(constants.ccwStencilPass);
		realDevice->mPipelineDepthStencilStateCreateInfo.back.compareOp = ConvertCompareOperation(constants.ccwStencilFunction);


		realDevice->mPipelineDepthStencilStateCreateInfo.front.failOp = ConvertStencilOperation(constants.stencilFail);
		realDevice->mPipelineDepthStencilStateCreateInfo.front.passOp = ConvertStencilOperation(constants.stencilPass);
		realDevice->mPipelineDepthStencilStateCreateInfo.front.compareOp = ConvertCompareOperation(constants.stencilFunction);
	}
	else
	{
		realDevice->mPipelineDepthStencilStateCreateInfo.back.failOp = ConvertStencilOperation(constants.stencilFail);
		realDevice->mPipelineDepthStencilStateCreateInfo.back.passOp = ConvertStencilOperation(constants.stencilPass);
		realDevice->mPipelineDepthStencilStateCreateInfo.back.compareOp = ConvertCompareOperation(constants.stencilFunction);

		realDevice->mPipelineDepthStencilStateCreateInfo.front.failOp = ConvertStencilOperation(constants.ccwStencilFail);
		realDevice->mPipelineDepthStencilStateCreateInfo.front.passOp = ConvertStencilOperation(constants.ccwStencilPass);
		realDevice->mPipelineDepthStencilStateCreateInfo.front.compareOp = ConvertCompareOperation(constants.ccwStencilFunction);
	}


	//mPipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
	//mPipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

	/**********************************************
	* Figure out correct shader
	**********************************************/
	realDevice->mGraphicsPipelineCreateInfo.stageCount = 2;

	if (context->VertexShader != nullptr)
	{
		realDevice->mPipelineShaderStageCreateInfo[0].module = mStateManager.mShaderConverters[context->VertexShader->mId]->mConvertedShader.ShaderModule;
		realDevice->mPipelineShaderStageCreateInfo[1].module = mStateManager.mShaderConverters[context->PixelShader->mId]->mConvertedShader.ShaderModule;
	}
	else
	{
		if (hasPosition && !hasColor && !hasNormal)
		{
			switch (textureCount)
			{
			case 0:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ;
				}

				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ;
				break;
			case 1:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW_TEX1;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_TEX1;
				}
		
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_TEX1;
				break;
			case 2:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW_TEX2;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_TEX2;
				}
		
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_TEX2;
				break;
			default:
				BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported texture count " << textureCount;
				break;
			}
		}
		else if (hasPosition && hasColor && !hasNormal)
		{
			switch (textureCount)
			{
			case 0:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW_DIFFUSE;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_DIFFUSE;
				}		

				if (deviceState.hasPointSpriteEnable)
				{
					realDevice->mGraphicsPipelineCreateInfo.stageCount = 3;
					realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_DIFFUSE_TEX1;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_DIFFUSE;
				}

				break;
			case 1:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW_DIFFUSE_TEX1;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_DIFFUSE_TEX1;
				}

				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_DIFFUSE_TEX1;
				break;
			case 2:
				if (isTransformed)
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZRHW_DIFFUSE_TEX2;
				}
				else
				{
					realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_DIFFUSE_TEX2;
				}
				
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_DIFFUSE_TEX2;
				break;
			default:
				BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported texture count " << textureCount;
				break;
			}
		}
		else if (hasPosition && hasColor && hasNormal)
		{
			switch (textureCount)
			{
			case 2:
				realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_NORMAL_DIFFUSE_TEX2;
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_NORMAL_DIFFUSE_TEX2;
				break;
			case 0:
				realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_NORMAL_DIFFUSE;
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_NORMAL_DIFFUSE;
				break;
			default:
				BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported texture count " << textureCount;
				break;
			}
		}
		else if (hasPosition && !hasColor && hasNormal)
		{
			switch (textureCount)
			{
			case 0:
				realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_NORMAL;
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_NORMAL;
				break;
			case 1:
				realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_NORMAL_TEX1;
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_NORMAL_TEX1;
				break;
			case 2:
				realDevice->mPipelineShaderStageCreateInfo[0].module = realDevice->mVertShaderModule_XYZ_NORMAL_TEX2;
				realDevice->mPipelineShaderStageCreateInfo[1].module = realDevice->mFragShaderModule_XYZ_NORMAL_TEX2;
				break;
			default:
				BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported texture count " << textureCount;
				break;
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe unsupported layout.";
			BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe hasPosition = " << hasPosition;
			BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe hasColor = " << hasColor;
			BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe hasNormal = " << hasNormal;
			BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreatePipe textureCount = " << textureCount;
		}
	}

	/**********************************************
	* Figure out attributes
	**********************************************/

	if (context->VertexDeclaration != nullptr)
	{
		uint32_t textureIndex = 0;

		attributeCount = context->VertexDeclaration->mVertexElements.size();

		for (size_t i = 0; i < attributeCount; i++)
		{
			D3DVERTEXELEMENT9& element = context->VertexDeclaration->mVertexElements[i];

			int t = D3DDECLTYPE_FLOAT3;

			realDevice->mVertexInputAttributeDescription[i].binding = element.Stream;
			//realDevice.mVertexInputAttributeDescription[i].location = location;
			realDevice->mVertexInputAttributeDescription[i].format = ConvertDeclType((D3DDECLTYPE)element.Type);
			realDevice->mVertexInputAttributeDescription[i].offset = element.Offset;

			switch ((D3DDECLUSAGE)element.Usage)
			{
			case D3DDECLUSAGE_POSITION:
				realDevice->mVertexInputAttributeDescription[i].location = 0;
				break;
			case D3DDECLUSAGE_BLENDWEIGHT:
				break;
			case D3DDECLUSAGE_BLENDINDICES:
				break;
			case D3DDECLUSAGE_NORMAL:
				realDevice->mVertexInputAttributeDescription[i].location = hasPosition;
				break;
			case D3DDECLUSAGE_PSIZE:
				break;
			case D3DDECLUSAGE_TEXCOORD:
				realDevice->mVertexInputAttributeDescription[i].location = hasPosition + hasNormal + hasColor + textureIndex;
				textureIndex += 1;
				break;
			case D3DDECLUSAGE_TANGENT:
				break;
			case D3DDECLUSAGE_BINORMAL:
				break;
			case D3DDECLUSAGE_TESSFACTOR:
				break;
			case D3DDECLUSAGE_POSITIONT:
				break;
			case D3DDECLUSAGE_COLOR:
				realDevice->mVertexInputAttributeDescription[i].location = hasPosition + hasNormal;
				break;
			case D3DDECLUSAGE_FOG:
				break;
			case D3DDECLUSAGE_DEPTH:
				break;
			case D3DDECLUSAGE_SAMPLE:
				break;
			default:
				break;
			}
		}
	}
	else if (context->FVF)
	{
		//TODO: revisit - make sure multiple sources is valid for FVF.
		for (int32_t i = 0; i < context->StreamCount; i++)
		{
			int attributeIndex = i * attributeCount;
			uint32_t offset = 0;
			uint32_t location = 0;

			if (hasPosition)
			{
				realDevice->mVertexInputAttributeDescription[attributeIndex].binding = i;
				realDevice->mVertexInputAttributeDescription[attributeIndex].location = location;
				realDevice->mVertexInputAttributeDescription[attributeIndex].format = vk::Format::eR32G32B32Sfloat;
				realDevice->mVertexInputAttributeDescription[attributeIndex].offset = offset;
				offset += (sizeof(float) * positionSize);
				location += 1;
				attributeIndex += 1;
			}

			if (hasNormal)
			{
				realDevice->mVertexInputAttributeDescription[attributeIndex].binding = i;
				realDevice->mVertexInputAttributeDescription[attributeIndex].location = location;
				realDevice->mVertexInputAttributeDescription[attributeIndex].format = vk::Format::eR32G32B32Sfloat;
				realDevice->mVertexInputAttributeDescription[attributeIndex].offset = offset;
				offset += (sizeof(float) * 3);
				location += 1;
				attributeIndex += 1;
			}

			//D3DFVF_PSIZE
			if ((context->FVF & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE)
			{
				realDevice->mVertexInputAttributeDescription[attributeIndex].binding = i;
				realDevice->mVertexInputAttributeDescription[attributeIndex].location = location;
				realDevice->mVertexInputAttributeDescription[attributeIndex].format = vk::Format::eB8G8R8A8Uint;
				realDevice->mVertexInputAttributeDescription[attributeIndex].offset = offset;
				offset += sizeof(uint32_t);
				location += 1;
				attributeIndex += 1;
			}

			if ((context->FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR)
			{
				realDevice->mVertexInputAttributeDescription[attributeIndex].binding = i;
				realDevice->mVertexInputAttributeDescription[attributeIndex].location = location;
				realDevice->mVertexInputAttributeDescription[attributeIndex].format = vk::Format::eB8G8R8A8Uint;
				realDevice->mVertexInputAttributeDescription[attributeIndex].offset = offset;
				offset += sizeof(uint32_t);
				location += 1;
				attributeIndex += 1;
			}

			for (size_t j = 0; j < textureCount; j++)
			{
				realDevice->mVertexInputAttributeDescription[attributeIndex].binding = i;
				realDevice->mVertexInputAttributeDescription[attributeIndex].location = location;
				realDevice->mVertexInputAttributeDescription[attributeIndex].format = vk::Format::eR32G32Sfloat;
				realDevice->mVertexInputAttributeDescription[attributeIndex].offset = offset;
				offset += (sizeof(float) * 2);
				location += 1;
				attributeIndex += 1;
			}
		}
	}
	else
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::BeginDraw unknown vertex format.";
	}

	realDevice->mPipelineLayoutCreateInfo.pPushConstantRanges = realDevice->mPushConstantRanges;
	realDevice->mPipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	if (context->VertexShader != nullptr)
	{
		auto& convertedVertexShader = mStateManager.mShaderConverters[context->VertexShader->mId]->mConvertedShader;
		auto& convertedPixelShader = mStateManager.mShaderConverters[context->PixelShader->mId]->mConvertedShader;

		realDevice->mPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = context->StreamCount;
		realDevice->mPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeCount;

		memcpy(&realDevice->mDescriptorSetLayoutBinding, &convertedPixelShader.mDescriptorSetLayoutBinding, sizeof(realDevice->mDescriptorSetLayoutBinding));

		realDevice->mDescriptorSetLayoutCreateInfo.pBindings = realDevice->mDescriptorSetLayoutBinding;
		realDevice->mPipelineLayoutCreateInfo.pSetLayouts = &context->DescriptorSetLayout;

		realDevice->mDescriptorSetLayoutCreateInfo.bindingCount = convertedPixelShader.mDescriptorSetLayoutBindingCount;
		realDevice->mPipelineLayoutCreateInfo.setLayoutCount = 1;

		realDevice->mVertexSpecializationInfo.pData = &context->mVertexShaderConstantSlots;
		realDevice->mVertexSpecializationInfo.dataSize = sizeof(ShaderConstantSlots);
		realDevice->mVertexSpecializationInfo.pMapEntries = realDevice->mSlotMapEntries;
		realDevice->mVertexSpecializationInfo.mapEntryCount = 1024;

		realDevice->mPixelSpecializationInfo.pData = &context->mPixelShaderConstantSlots;
		realDevice->mPixelSpecializationInfo.dataSize = sizeof(ShaderConstantSlots);
		realDevice->mPixelSpecializationInfo.pMapEntries = realDevice->mSlotMapEntries;
		realDevice->mPixelSpecializationInfo.mapEntryCount = 1024;
	}
	else
	{
		realDevice->mPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = context->StreamCount;
		realDevice->mPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeCount;

		realDevice->mDescriptorSetLayoutBinding[0].binding = 0;
		realDevice->mDescriptorSetLayoutBinding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		realDevice->mDescriptorSetLayoutBinding[0].descriptorCount = 1;
		realDevice->mDescriptorSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
		realDevice->mDescriptorSetLayoutBinding[0].pImmutableSamplers = nullptr;

		realDevice->mDescriptorSetLayoutBinding[1].binding = 1;
		realDevice->mDescriptorSetLayoutBinding[1].descriptorType = vk::DescriptorType::eUniformBuffer;
		realDevice->mDescriptorSetLayoutBinding[1].descriptorCount = 1;
		realDevice->mDescriptorSetLayoutBinding[1].stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
		realDevice->mDescriptorSetLayoutBinding[1].pImmutableSamplers = nullptr;

		realDevice->mDescriptorSetLayoutBinding[2].binding = 2;
		realDevice->mDescriptorSetLayoutBinding[2].descriptorType = vk::DescriptorType::eCombinedImageSampler; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER'
		realDevice->mDescriptorSetLayoutBinding[2].descriptorCount = constants.textureCount; //Update to use mapped texture.
		realDevice->mDescriptorSetLayoutBinding[2].stageFlags = vk::ShaderStageFlagBits::eFragment;
		realDevice->mDescriptorSetLayoutBinding[2].pImmutableSamplers = nullptr;

		realDevice->mDescriptorSetLayoutCreateInfo.pBindings = realDevice->mDescriptorSetLayoutBinding;
		realDevice->mPipelineLayoutCreateInfo.pSetLayouts = &context->DescriptorSetLayout;
		realDevice->mPipelineLayoutCreateInfo.setLayoutCount = 1;

		if (constants.textureCount)
		{
			realDevice->mDescriptorSetLayoutCreateInfo.bindingCount = 3; //The number of elements in pBindings.			
		}
		else
		{
			realDevice->mDescriptorSetLayoutCreateInfo.bindingCount = 2; //The number of elements in pBindings.	
		}

		realDevice->mVertexSpecializationInfo.pData = &deviceState.mSpecializationConstants;
		realDevice->mVertexSpecializationInfo.dataSize = sizeof(SpecializationConstants);
		realDevice->mVertexSpecializationInfo.pMapEntries = realDevice->mSlotMapEntries;
		realDevice->mVertexSpecializationInfo.mapEntryCount = 251;

		realDevice->mPixelSpecializationInfo.pData = &deviceState.mSpecializationConstants;
		realDevice->mPixelSpecializationInfo.dataSize = sizeof(SpecializationConstants);
		realDevice->mPixelSpecializationInfo.pMapEntries = realDevice->mSlotMapEntries;
		realDevice->mPixelSpecializationInfo.mapEntryCount = 251;
	}

	result = device.createDescriptorSetLayout(&realDevice->mDescriptorSetLayoutCreateInfo, nullptr, &context->DescriptorSetLayout);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::CreateDescriptorSet vkCreateDescriptorSetLayout failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	/**********************************************
	* Create pipeline & descriptor set layout.
	**********************************************/

	result = device.createPipelineLayout(&realDevice->mPipelineLayoutCreateInfo, nullptr, &context->PipelineLayout);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::BeginDraw vkCreatePipelineLayout failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mGraphicsPipelineCreateInfo.layout = context->PipelineLayout;
	realDevice->mGraphicsPipelineCreateInfo.renderPass = realDevice->mDeviceState.mRenderTarget->mStoreRenderPass;

	result = device.createGraphicsPipelines(realDevice->mPipelineCache, 1, &realDevice->mGraphicsPipelineCreateInfo, nullptr, &context->Pipeline);
	//result = vkCreateGraphicsPipelines(mDevice->mDevice, VK_NULL_HANDLE, 1, &mGraphicsPipelineCreateInfo, nullptr, &context.Pipeline);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::BeginDraw vkCreateGraphicsPipelines failed with return code of " << GetResultString((VkResult)result);
	}

	realDevice->mDrawBuffer.push_back(context);
}

void RenderManager::CreateSampler(std::shared_ptr<RealDevice> realDevice, std::shared_ptr<SamplerRequest> request)
{
	//https://msdn.microsoft.com/en-us/library/windows/desktop/bb172602(v=vs.85).aspx
	//Mipmap filter to use during minification. See D3DTEXTUREFILTERTYPE. The default value is D3DTEXF_NONE.

	vk::Result result;
	//auto& deviceState = realDevice.mDeviceState;
	auto& device = realDevice->mDevice;

	vk::SamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.magFilter = ConvertFilter(request->MagFilter);
	samplerCreateInfo.minFilter = ConvertFilter(request->MinFilter);
	samplerCreateInfo.addressModeU = ConvertTextureAddress(request->AddressModeU);
	samplerCreateInfo.addressModeV = ConvertTextureAddress(request->AddressModeV);
	samplerCreateInfo.addressModeW = ConvertTextureAddress(request->AddressModeW);
	samplerCreateInfo.mipmapMode = ConvertMipmapMode(request->MipmapMode); //VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.mipLodBias = request->MipLodBias;
	//samplerCreateInfo.compareEnable = true;

	/*
	https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html
	If either magFilter or minFilter is VK_FILTER_CUBIC_IMG, anisotropyEnable must be VK_FALSE
	*/
	if (realDevice->mPhysicalDeviceFeatures.samplerAnisotropy && samplerCreateInfo.minFilter != vk::Filter::eCubicIMG && samplerCreateInfo.magFilter != vk::Filter::eCubicIMG)
	{
		// Use max. level of anisotropy for this example
		samplerCreateInfo.maxAnisotropy = std::min((float)request->MaxAnisotropy, realDevice->mPhysicalDeviceProperties.limits.maxSamplerAnisotropy);

		if (request->MinFilter == D3DTEXF_ANISOTROPIC ||
			request->MagFilter == D3DTEXF_ANISOTROPIC ||
			request->MipmapMode == D3DTEXF_ANISOTROPIC)
		{
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
		}
		else {
			samplerCreateInfo.anisotropyEnable = VK_FALSE;
		}
	}
	else
	{
		// The device does not support anisotropic filtering or cubic is currently in use.
		samplerCreateInfo.maxAnisotropy = 1.0;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
	}

	samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite; // VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareOp = vk::CompareOp::eNever; //VK_COMPARE_OP_ALWAYS
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = request->MaxLod;

	if (request->MipmapMode == D3DTEXF_NONE)
	{
		samplerCreateInfo.maxLod = 0.0f;
	}

	result = device.createSampler(&samplerCreateInfo, nullptr, &request->Sampler);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RenderManager::GenerateSampler vkCreateSampler failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mSamplerRequests.push_back(request);
}

void RenderManager::UpdatePushConstants(std::shared_ptr<RealDevice> realDevice, std::shared_ptr<DrawContext> context)
{
	//vk::Result result;
	auto& deviceState = realDevice->mDeviceState;
	//auto& device = realDevice.mRealDevice.mDevice;
	auto& currentSwapChainBuffer = realDevice->mCommandBuffers[realDevice->mCurrentCommandBuffer];
	void* data = nullptr;

	//if (!mDevice->mDeviceState.mHasTransformsChanged)
	//{
	//	return;
	//}

	realDevice->mTransformations.mModel <<
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1;

	realDevice->mTransformations.mView <<
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1;

	realDevice->mTransformations.mProjection <<
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1;

	BOOST_FOREACH(const auto& pair1, deviceState.mTransforms)
	{
		switch (pair1.first)
		{
		case D3DTS_WORLD:

			realDevice->mTransformations.mModel <<
				pair1.second.m[0][0], pair1.second.m[1][0], pair1.second.m[2][0], pair1.second.m[3][0],
				pair1.second.m[0][1], pair1.second.m[1][1], pair1.second.m[2][1], pair1.second.m[3][1],
				pair1.second.m[0][2], pair1.second.m[1][2], pair1.second.m[2][2], pair1.second.m[3][2],
				pair1.second.m[0][3], pair1.second.m[1][3], pair1.second.m[2][3], pair1.second.m[3][3];

			break;
		case D3DTS_VIEW:

			realDevice->mTransformations.mView <<
				pair1.second.m[0][0], pair1.second.m[1][0], pair1.second.m[2][0], pair1.second.m[3][0],
				pair1.second.m[0][1], pair1.second.m[1][1], pair1.second.m[2][1], pair1.second.m[3][1],
				pair1.second.m[0][2], pair1.second.m[1][2], pair1.second.m[2][2], pair1.second.m[3][2],
				pair1.second.m[0][3], pair1.second.m[1][3], pair1.second.m[2][3], pair1.second.m[3][3];

			break;
		case D3DTS_PROJECTION:

			realDevice->mTransformations.mProjection <<
				pair1.second.m[0][0], pair1.second.m[1][0], pair1.second.m[2][0], pair1.second.m[3][0],
				pair1.second.m[0][1], pair1.second.m[1][1], pair1.second.m[2][1], pair1.second.m[3][1],
				pair1.second.m[0][2], pair1.second.m[1][2], pair1.second.m[2][2], pair1.second.m[3][2],
				pair1.second.m[0][3], pair1.second.m[1][3], pair1.second.m[2][3], pair1.second.m[3][3];

			break;
		default:
			BOOST_LOG_TRIVIAL(warning) << "RenderManager::UpdateUniformBuffer The following state type was ignored. " << pair1.first;
			break;
		}
	}

	realDevice->mTransformations.mTotalTransformation = realDevice->mTransformations.mProjection * realDevice->mTransformations.mView * realDevice->mTransformations.mModel;
	//mTotalTransformation = mModel * mView * mProjection;

	currentSwapChainBuffer.pushConstants(context->PipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, UBO_SIZE * 2, &realDevice->mTransformations);
}

void RenderManager::FlushDrawBufffer(std::shared_ptr<RealDevice> realDevice)
{
	/*
	Uses remove_if and chrono to remove elements that have not been used in over a second.
	*/
	realDevice->mDrawBuffer.erase(std::remove_if(realDevice->mDrawBuffer.begin(), realDevice->mDrawBuffer.end(), [](const std::shared_ptr<DrawContext> & o) { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - o->LastUsed).count() > CACHE_SECONDS; }), realDevice->mDrawBuffer.end());
	realDevice->mSamplerRequests.erase(std::remove_if(realDevice->mSamplerRequests.begin(), realDevice->mSamplerRequests.end(), [](const std::shared_ptr<SamplerRequest> & o) { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - o->LastUsed).count() > CACHE_SECONDS; }), realDevice->mSamplerRequests.end());

	realDevice->mRenderTargets.clear();

	realDevice->mIsDirty = true;
}
