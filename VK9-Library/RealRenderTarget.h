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
#include "d3d9.h"
#include "CTypes.h" //Needed for DeviceState

struct RealSurface;
struct RealTexture;

#ifndef REALRENDERTARGET_H
#define REALRENDERTARGET_H

struct RealRenderTarget
{
	vk::Device mDevice;
	RealTexture* mColorTexture = nullptr;
	RealSurface* mColorSurface = nullptr;
	RealSurface* mDepthSurface = nullptr;

	RealRenderTarget(vk::Device device, RealTexture* colorTexture, RealSurface* colorSurface, RealSurface* depthSurface);
	RealRenderTarget(vk::Device device, RealSurface* colorSurface, RealSurface* depthSurface);
	~RealRenderTarget();

	bool mIsSceneStarted = false;
	vk::RenderPass mStoreRenderPass;
	vk::RenderPass mClearRenderPass;
	vk::AttachmentDescription mRenderAttachments[2];
	vk::ClearValue mClearValues[2];
	vk::ColorSpaceKHR mColorSpace;
	vk::ClearColorValue mClearColorValue;
	vk::ClearDepthStencilValue mClearDepthValue;
	vk::Framebuffer mFramebuffer;
	vk::RenderPassBeginInfo mRenderPassBeginInfo;
	vk::ImageMemoryBarrier mImageMemoryBarrier;
	vk::ImageMemoryBarrier mPrePresentBarrier;
	vk::PipelineStageFlags mPipeStageFlags;
	vk::SemaphoreCreateInfo mPresentCompleteSemaphoreCreateInfo;
	vk::Semaphore mPresentCompleteSemaphore;
	vk::SubmitInfo mSubmitInfo;
	vk::Fence mNullFence;
	vk::Fence mCommandFence;
	vk::CommandBufferBeginInfo mCommandBufferBeginInfo;

	void StartScene(vk::CommandBuffer command, DeviceState& deviceState, bool clear, bool createNewCommand);
	void StopScene(vk::CommandBuffer command, vk::Queue queue);
	void Clear(vk::CommandBuffer command, DeviceState& deviceState, DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
};

#endif // REALRENDERTARGET_H