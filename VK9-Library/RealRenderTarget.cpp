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

#include "RealRenderTarget.h"

RealRenderTarget::RealRenderTarget(vk::Device device, RealTexture* colorTexture, RealSurface* colorSurface, RealSurface* depthSurface)
	: mDevice(device),
	mColorTexture(colorTexture),
	mColorSurface(colorSurface),
	mDepthSurface(depthSurface)
{
	if (colorTexture == nullptr || depthSurface == nullptr)
	{
		return;
	}
	//BOOST_LOG_TRIVIAL(info) << "RealRenderTarget::RealRenderTarget";

	vk::Result result;

	mCommandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::eGeneral; //eColorAttachmentOptimal

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout = vk::ImageLayout::eGeneral; //eDepthStencilAttachmentOptimal

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	mRenderAttachments[0].format = mColorTexture->mRealFormat;
	mRenderAttachments[0].samples = vk::SampleCountFlagBits::e1;
	mRenderAttachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[0].stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[0].stencilStoreOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[0].initialLayout = vk::ImageLayout::eGeneral;
	mRenderAttachments[0].finalLayout = vk::ImageLayout::eGeneral;

	mRenderAttachments[1].format = mDepthSurface->mRealFormat;
	mRenderAttachments[1].samples = vk::SampleCountFlagBits::e1;
	mRenderAttachments[1].loadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[1].storeOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[1].stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[1].stencilStoreOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[1].initialLayout = vk::ImageLayout::eGeneral;
	mRenderAttachments[1].finalLayout = vk::ImageLayout::eGeneral;

	vk::SubpassDependency dependency;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;

	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.attachmentCount = 2; //revisit
	renderPassCreateInfo.pAttachments = mRenderAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	result = mDevice.createRenderPass(&renderPassCreateInfo, nullptr, &mStoreRenderPass);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateRenderPass failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mRenderAttachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	mRenderAttachments[1].loadOp = vk::AttachmentLoadOp::eClear;

	result = mDevice.createRenderPass(&renderPassCreateInfo, nullptr, &mClearRenderPass);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateRenderPass failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::ImageView attachments[2];
	attachments[0] = mColorTexture->mImageView;
	attachments[1] = mDepthSurface->mStagingImageView;

	vk::FramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.renderPass = mStoreRenderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = attachments;
	framebufferCreateInfo.width = mColorTexture->mExtent.width;
	framebufferCreateInfo.height = mColorTexture->mExtent.height;
	framebufferCreateInfo.layers = 1;

	result = mDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &mFramebuffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateFramebuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mClearValues[0].color = mClearColorValue;
	mClearValues[1].depthStencil = mClearDepthValue; //vk::ClearDepthStencilValue(1.0f, 0);

	mRenderPassBeginInfo.framebuffer = mFramebuffer;
	mRenderPassBeginInfo.renderArea.offset.x = 0;
	mRenderPassBeginInfo.renderArea.offset.y = 0;
	mRenderPassBeginInfo.renderArea.extent.width = mColorTexture->mExtent.width;
	mRenderPassBeginInfo.renderArea.extent.height = mColorTexture->mExtent.height;
	mRenderPassBeginInfo.clearValueCount = 2; //2
	mRenderPassBeginInfo.pClearValues = mClearValues;

	//realWindow.mImageMemoryBarrier.srcAccessMask = 0;
	mImageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	mImageMemoryBarrier.oldLayout = vk::ImageLayout::eGeneral; //eUndefined
	mImageMemoryBarrier.newLayout = vk::ImageLayout::eGeneral; //ePresentSrcKHR
	mImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mImageMemoryBarrier.image = mColorTexture->mImage;
	mImageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	result = mDevice.createSemaphore(&mPresentCompleteSemaphoreCreateInfo, nullptr, &mPresentCompleteSemaphore);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "StateManager::CreateWindow1 vkCreateSemaphore failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::FenceCreateInfo fenceInfo;

	mDevice.createFence(&fenceInfo, nullptr, &mCommandFence);
}

RealRenderTarget::RealRenderTarget(vk::Device device, RealSurface* colorSurface, RealSurface* depthSurface)
	: mDevice(device),
	mColorSurface(colorSurface),
	mDepthSurface(depthSurface)
{
	if (colorSurface == nullptr || depthSurface == nullptr)
	{
		return;
	}
	if (mDepthSurface->mExtent.width != mColorSurface->mExtent.width)
	{
		return;
	}

	//BOOST_LOG_TRIVIAL(info) << "RealRenderTarget::RealRenderTarget";

	vk::Result result;

	mCommandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::eGeneral; //eColorAttachmentOptimal

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout = vk::ImageLayout::eGeneral; //eDepthStencilAttachmentOptimal

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	mRenderAttachments[0].format = mColorSurface->mRealFormat;
	mRenderAttachments[0].samples = vk::SampleCountFlagBits::e1;
	mRenderAttachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[0].stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[0].stencilStoreOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[0].initialLayout = vk::ImageLayout::eGeneral; // vk::ImageLayout::eUndefined;  ePresentSrcKHR
	mRenderAttachments[0].finalLayout = vk::ImageLayout::eGeneral;

	mRenderAttachments[1].format = mDepthSurface->mRealFormat;
	mRenderAttachments[1].samples = vk::SampleCountFlagBits::e1;
	mRenderAttachments[1].loadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[1].storeOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[1].stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	mRenderAttachments[1].stencilStoreOp = vk::AttachmentStoreOp::eStore;
	mRenderAttachments[1].initialLayout = vk::ImageLayout::eGeneral; //vk::ImageLayout::eUndefined; eDepthStencilAttachmentOptimal
	mRenderAttachments[1].finalLayout = vk::ImageLayout::eGeneral;

	vk::SubpassDependency dependency;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;

	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.attachmentCount = 2; //revisit
	renderPassCreateInfo.pAttachments = mRenderAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	result = mDevice.createRenderPass(&renderPassCreateInfo, nullptr, &mStoreRenderPass);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateRenderPass failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mRenderAttachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	mRenderAttachments[1].loadOp = vk::AttachmentLoadOp::eClear;

	result = mDevice.createRenderPass(&renderPassCreateInfo, nullptr, &mClearRenderPass);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateRenderPass failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::ImageView attachments[2];
	attachments[0] = mColorSurface->mStagingImageView;
	attachments[1] = mDepthSurface->mStagingImageView;

	vk::FramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.renderPass = mStoreRenderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = attachments;
	framebufferCreateInfo.width = mColorSurface->mExtent.width;
	framebufferCreateInfo.height = mColorSurface->mExtent.height;
	framebufferCreateInfo.layers = 1;

	result = mDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &mFramebuffer);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::RealRenderTarget vkCreateFramebuffer failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	mClearValues[0].color = mClearColorValue;
	mClearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	mRenderPassBeginInfo.framebuffer = mFramebuffer;
	mRenderPassBeginInfo.renderArea.offset.x = 0;
	mRenderPassBeginInfo.renderArea.offset.y = 0;
	mRenderPassBeginInfo.renderArea.extent.width = mColorSurface->mExtent.width;
	mRenderPassBeginInfo.renderArea.extent.height = mColorSurface->mExtent.height;
	mRenderPassBeginInfo.clearValueCount = 2; //2
	mRenderPassBeginInfo.pClearValues = mClearValues;

	//realWindow.mImageMemoryBarrier.srcAccessMask = 0;
	mImageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	mImageMemoryBarrier.oldLayout = vk::ImageLayout::eGeneral; //eUndefined
	mImageMemoryBarrier.newLayout = vk::ImageLayout::eGeneral; //ePresentSrcKHR
	mImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mImageMemoryBarrier.image = mColorSurface->mStagingImage;
	mImageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	result = mDevice.createSemaphore(&mPresentCompleteSemaphoreCreateInfo, nullptr, &mPresentCompleteSemaphore);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "StateManager::CreateWindow1 vkCreateSemaphore failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	vk::FenceCreateInfo fenceInfo;

	mDevice.createFence(&fenceInfo, nullptr, &mCommandFence);
}

RealRenderTarget::~RealRenderTarget()
{
	BOOST_LOG_TRIVIAL(info) << "RealRenderTarget::~RealRenderTarget";
	mDevice.destroyFence(mCommandFence, nullptr);
	mDevice.destroySemaphore(mPresentCompleteSemaphore, nullptr);
	mDevice.destroyFramebuffer(mFramebuffer, nullptr);
	mDevice.destroyRenderPass(mStoreRenderPass, nullptr);
	mDevice.destroyRenderPass(mClearRenderPass, nullptr);
}

void RealRenderTarget::StartScene(vk::CommandBuffer command, DeviceState& deviceState, bool clear, bool createNewCommand)
{
	mIsSceneStarted = true;

	if (clear)
	{
		mClearValues[0].color = mClearColorValue;
		mRenderPassBeginInfo.renderPass = mClearRenderPass;
	}
	else
	{
		mRenderPassBeginInfo.renderPass = mStoreRenderPass;
	}

	if (createNewCommand)
	{
		vk::Result result = command.begin(&mCommandBufferBeginInfo);
		if (result != vk::Result::eSuccess)
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealRenderTarget::StartScene vkBeginCommandBuffer failed with return code of " << GetResultString((VkResult)result);
			return;
		}
	}

	command.setViewport(0, 1, &deviceState.mViewport);
	command.setScissor(0, 1, &deviceState.mScissor);

	ReallySetImageLayout(command, mColorSurface->mStagingImage, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, 0, 1); //ePresentSrcKHR
	
	auto& realFormat = mDepthSurface->mRealFormat;
	if (realFormat == vk::Format::eD16UnormS8Uint || realFormat == vk::Format::eD24UnormS8Uint || realFormat == vk::Format::eD32SfloatS8Uint)
	{
		ReallySetImageLayout(command, mDepthSurface->mStagingImage, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, 0, 1); //eDepthStencilAttachmentOptimal
	}
	else if (realFormat == vk::Format::eS8Uint)
	{
		ReallySetImageLayout(command, mDepthSurface->mStagingImage, vk::ImageAspectFlagBits::eStencil, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, 0, 1); //eDepthStencilAttachmentOptimal
	}
	else
	{
		ReallySetImageLayout(command, mDepthSurface->mStagingImage, vk::ImageAspectFlagBits::eDepth, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, 0, 1); //eDepthStencilAttachmentOptimal
	}	

	command.beginRenderPass(&mRenderPassBeginInfo, vk::SubpassContents::eInline);

	//Set the pass back to store so draw calls won't be lost if they require stop/start of render pass.
	mRenderPassBeginInfo.renderPass = mStoreRenderPass;
}

void RealRenderTarget::StopScene(vk::CommandBuffer command, vk::Queue queue)
{
	mIsSceneStarted = false;

	command.endRenderPass();

	mPipeStageFlags = vk::PipelineStageFlagBits::eAllCommands; //VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
}

void RealRenderTarget::Clear(vk::CommandBuffer command, DeviceState& deviceState, DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	if ((Flags & D3DCLEAR_TARGET) == D3DCLEAR_TARGET)
	{
		//VK_FORMAT_B8G8R8A8_UNORM 
		//Revisit the byte order could be difference based on the surface format so I need a better way to handle this.
		mClearColorValue.float32[3] = D3DCOLOR_A(Color); //FLT_MAX; 
		mClearColorValue.float32[2] = D3DCOLOR_B(Color);
		mClearColorValue.float32[1] = D3DCOLOR_G(Color);
		mClearColorValue.float32[0] = D3DCOLOR_R(Color);
	}

	if ((Flags & D3DCLEAR_STENCIL) == D3DCLEAR_STENCIL || (Flags & D3DCLEAR_ZBUFFER) == D3DCLEAR_ZBUFFER)
	{
		mClearDepthValue = vk::ClearDepthStencilValue(Z, Stencil);
	}

	if (Count > 0 && pRects != nullptr)
	{
		BOOST_LOG_TRIVIAL(warning) << "RealRenderTarget::Clear is not fully implemented!";
		return;
	}

	vk::ImageSubresourceRange subResourceRange;
	//subResourceRange.aspectMask = vk::ImageAspectFlagBits::eColor | vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	if (mIsSceneStarted)
	{
		command.endRenderPass();

		if ((Flags & D3DCLEAR_TARGET) == D3DCLEAR_TARGET)
		{
			subResourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

			//ReallySetImageLayout(command, mColorSurface->mStagingImage, subResourceRange.aspectMask, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);
			command.clearColorImage(mColorSurface->mStagingImage, vk::ImageLayout::eGeneral, &mClearColorValue, 1, &subResourceRange);
			//ReallySetImageLayout(command, mColorSurface->mStagingImage, subResourceRange.aspectMask, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, 1, 0, 1);
		}

		if ((Flags & D3DCLEAR_STENCIL) == D3DCLEAR_STENCIL || (Flags & D3DCLEAR_ZBUFFER) == D3DCLEAR_ZBUFFER)
		{
			subResourceRange.aspectMask = vk::ImageAspectFlags();

			if ((Flags & D3DCLEAR_STENCIL) == D3DCLEAR_STENCIL)
			{
				subResourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
			}

			if ((Flags & D3DCLEAR_ZBUFFER) == D3DCLEAR_ZBUFFER)
			{
				subResourceRange.aspectMask |= vk::ImageAspectFlagBits::eDepth;
			}

			//ReallySetImageLayout(command, mDepthSurface->mStagingImage, subResourceRange.aspectMask, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);
			command.clearDepthStencilImage(mDepthSurface->mStagingImage, vk::ImageLayout::eGeneral, &mClearDepthValue, 1, &subResourceRange);
			//ReallySetImageLayout(command, mDepthSurface->mStagingImage, subResourceRange.aspectMask, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, 1, 0, 1);
		}

		command.beginRenderPass(&mRenderPassBeginInfo, vk::SubpassContents::eInline);
	}
	else
	{
		this->StartScene(command, deviceState, true, deviceState.hasPresented);
	}
}