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

#ifndef REALSWAPCHAIN_H
#define REALSWAPCHAIN_H


struct RealSwapChain
{
	//Parameters
	vk::Instance mInstance;
	vk::PhysicalDevice mPhysicalDevice;
	vk::Device mDevice;
	HWND mWindowHandle;
	uint32_t mWidth;
	uint32_t mHeight;

	//Surface Stuff
	vk::SurfaceKHR mSurface;
	vk::SurfaceCapabilitiesKHR mSurfaceCapabilities;
	uint32_t mSurfaceFormatCount;
	vk::SurfaceFormatKHR* mSurfaceFormats;
	vk::SurfaceTransformFlagBitsKHR mTransformFlags;
	vk::Format mSurfaceFormat;
	vk::PresentModeKHR mPresentationMode;
	vk::PresentInfoKHR mPresentInfo;

	//SwapChain Stuff
	vk::SwapchainKHR mSwapchain;
	vk::Extent2D mSwapchainExtent;
	uint32_t mSwapchainImageCount;
	vk::Image* mImages;
	vk::ImageView* mViews;
	

	//DepthBuffer
	vk::Image mDepthImage;
	vk::ImageView mDepthView;
	vk::DeviceMemory mDepthDeviceMemory;
	vk::Format mDepthFormat = vk::Format::eD16Unorm;

	//Presentation
	vk::SemaphoreCreateInfo mPresentCompleteSemaphoreCreateInfo;
	
	vk::CommandBufferBeginInfo mCommandBufferBeginInfo;
	vk::PipelineStageFlags mPipeStageFlags;
	vk::SubmitInfo mSubmitInfo;
	vk::ImageMemoryBarrier mPrePresentBarrier;
	vk::ImageMemoryBarrier mImageMemoryBarrier;

	//Misc
	vk::Result mResult;
	uint32_t mCurrentIndex=0;
	vk::Fence mNullFence;
	vk::Fence mSwapFence;

	vk::Semaphore mSwapSemaphore;

	//Functions
	RealSwapChain(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device, HWND windowHandle, uint32_t width, uint32_t height);
	~RealSwapChain();

	void InitSurface();
	void DestroySurface();
	void InitSwapChain();
	void DestroySwapChain();
	void InitDepthBuffer();
	void DestroyDepthBuffer();

	void Present(vk::CommandBuffer commandBuffer, vk::Queue queue, vk::Image source);

};

#endif // REALSWAPCHAIN_H