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

#include "RealSurface.h"
#include "CSurface9.h"
#include "CVolume9.h"

#include "Utilities.h"

RealSurface::RealSurface(RealDevice* realDevice, CSurface9* surface9, vk::Image* parentImage)
	: mRealDevice(realDevice)
{
	BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface";

	vk::Result result;

	mRealFormat = ConvertFormat(surface9->mFormat);

	if (mRealFormat == vk::Format::eUndefined)//VK_FORMAT_UNDEFINED
	{
		if (surface9->mFormat > 199)
		{
			char four[5] =
			{
				(char)(surface9->mFormat & 0xFF),
				(char)((surface9->mFormat >> 8) & 0xFF),
				(char)((surface9->mFormat >> 16) & 0xFF),
				(char)((surface9->mFormat >> 24) & 0xFF),
				'\0'
			};

			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface unknown format: " << four;
		}
		else
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface unknown format: " << surface9->mFormat;
		}

	}

	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.format = mRealFormat; //VK_FORMAT_B8G8R8A8_UNORM
	imageCreateInfo.extent = vk::Extent3D(surface9->mWidth, surface9->mHeight, 1);
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

	if (surface9->mTexture != nullptr || surface9->mCubeTexture != nullptr)
	{
		imageCreateInfo.tiling = vk::ImageTiling::eLinear;
		imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

		imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
	}
	else
	{
		imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
		imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized; //ePreinitialized

		if (surface9->mUsage == D3DUSAGE_DEPTHSTENCIL)
		{
			imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eDepthStencilAttachment;
		}
		else
		{
			imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment; //
		}
	}

	mExtent = imageCreateInfo.extent;

	//if (surface9->mCubeTexture != nullptr)
	//{
	//	imageCreateInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	//}

	result = realDevice->mDevice.createImage(&imageCreateInfo, nullptr, &mStagingImage);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage failed with return code of " << GetResultString((VkResult)result);
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage format:" << (VkFormat)imageCreateInfo.format;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage imageType:" << (VkImageType)imageCreateInfo.imageType;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage tiling:" << (VkImageTiling)imageCreateInfo.tiling;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage usage:" << (VkImageUsageFlags)imageCreateInfo.usage;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage flags:" << (VkImageCreateFlags)imageCreateInfo.flags;
		return;
	}

	//BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface vkCreateImage: " << static_cast<uint64_t>(mStagingImage);

	//vk::DebugMarkerObjectNameInfoEXT objectName;
	//objectName.object = static_cast<uint64_t>(mStagingImage);
	//objectName.objectType = vk::DebugReportObjectTypeEXT::eImage;

	//realDevice->mDevice.debugMarkerSetObjectNameEXT(objectName);

	vk::MemoryRequirements memoryRequirements;
	realDevice->mDevice.getImageMemoryRequirements(mStagingImage, &memoryRequirements);

	//mMemoryAllocateInfo.allocationSize = 0;
	mMemoryAllocateInfo.memoryTypeIndex = 0;
	mMemoryAllocateInfo.allocationSize = memoryRequirements.size;

	if (surface9->mTexture != nullptr || surface9->mCubeTexture != nullptr)
	{
		if (!GetMemoryTypeFromProperties(realDevice->mPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &mMemoryAllocateInfo.memoryTypeIndex))
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface Could not find memory type from properties.";
			return;
		}
	}
	else
	{
		if (!GetMemoryTypeFromProperties(realDevice->mPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, 0, &mMemoryAllocateInfo.memoryTypeIndex))
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface Could not find memory type from properties.";
			return;
		}
	}

	result = realDevice->mDevice.allocateMemory(&mMemoryAllocateInfo, nullptr, &mStagingDeviceMemory);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkAllocateMemory failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mDevice.bindImageMemory(mStagingImage, mStagingDeviceMemory, 0);

	mSubresource.mipLevel = 0;

	vk::ImageViewCreateInfo imageViewCreateInfo;

	if (mRealFormat == vk::Format::eD16UnormS8Uint || mRealFormat == vk::Format::eD24UnormS8Uint || mRealFormat == vk::Format::eD32SfloatS8Uint)
	{
		mSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}
	else if (mRealFormat == vk::Format::eS8Uint)
	{
		mSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
	}
	else if (mRealFormat == vk::Format::eD16Unorm)
	{
		mSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	}
	else
	{
		mSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	mSubresource.arrayLayer = 0; //if this is wrong you may get 4294967296.

	mLayouts[0] = {};
	if (imageCreateInfo.tiling == vk::ImageTiling::eLinear)
	{
		//BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface (CSurface9) using format " << (VkFormat)mRealFormat;
		realDevice->mDevice.getImageSubresourceLayout(mStagingImage, &mSubresource, &mLayouts[0]);
	}

	imageViewCreateInfo.image = mStagingImage;
	//imageViewCreateInfo.viewType = vk::ImageViewType::e3D;
	imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imageViewCreateInfo.format = mRealFormat;

	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	/*
	This block handles the luminance & x formats. They are converted to color formats but need a little mapping to make them work correctly.
	*/
	if (surface9->mTexture != nullptr || surface9->mCubeTexture != nullptr)
	{
		switch (surface9->mFormat)
		{
		case D3DFMT_L8:
			imageViewCreateInfo.components = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eOne);
			break;
		case D3DFMT_A8L8:
			imageViewCreateInfo.components = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG);
			break;
		case D3DFMT_X8R8G8B8:
		case D3DFMT_X8B8G8R8:
			imageViewCreateInfo.components = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eOne);
			break;
		default:
			break;
		}
	}
	else
	{
		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
	}

	if (surface9->mTexture == nullptr && surface9->mCubeTexture == nullptr)
	{
		result = realDevice->mDevice.createImageView(&imageViewCreateInfo, nullptr, &mStagingImageView);
		if (result != vk::Result::eSuccess)
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImageView failed with return code of " << GetResultString((VkResult)result);
			return;
		}
	}
	else if (parentImage != nullptr)
	{
		imageViewCreateInfo.image = (*parentImage);

		result = realDevice->mDevice.createImageView(&imageViewCreateInfo, nullptr, &mStagingImageView);
		if (result != vk::Result::eSuccess)
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImageView failed with return code of " << GetResultString((VkResult)result);
			return;
		}
	}
}

RealSurface::RealSurface(RealDevice* realDevice, CVolume9* volume9)
	: mRealDevice(realDevice)
{
	BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface";

	vk::Result result;

	mRealFormat = ConvertFormat(volume9->mFormat);

	if (mRealFormat == vk::Format::eUndefined)//VK_FORMAT_UNDEFINED
	{
		if (volume9->mFormat > 199)
		{
			char four[5] =
			{
				(char)(volume9->mFormat & 0xFF),
				(char)((volume9->mFormat >> 8) & 0xFF),
				(char)((volume9->mFormat >> 16) & 0xFF),
				(char)((volume9->mFormat >> 24) & 0xFF),
				'\0'
			};

			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface unknown format: " << four;
		}
		else
		{
			BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface unknown format: " << volume9->mFormat;
		}

	}

	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.imageType = vk::ImageType::e3D;
	imageCreateInfo.format = mRealFormat;
	imageCreateInfo.extent = vk::Extent3D(volume9->mWidth, volume9->mHeight, volume9->mDepth);
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imageCreateInfo.tiling = vk::ImageTiling::eLinear;
	imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
	imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

	//if (Volume9->mCubeTexture != nullptr)
	//{
	//	imageCreateInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	//}

	result = realDevice->mDevice.createImage(&imageCreateInfo, nullptr, &mStagingImage);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage failed with return code of " << GetResultString((VkResult)result);
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage format:" << (VkFormat)imageCreateInfo.format;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage imageType:" << (VkImageType)imageCreateInfo.imageType;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage tiling:" << (VkImageTiling)imageCreateInfo.tiling;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage usage:" << (VkImageUsageFlags)imageCreateInfo.usage;
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkCreateImage flags:" << (VkImageCreateFlags)imageCreateInfo.flags;
		return;
	}

	//BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface vkCreateImage: " << static_cast<uint64_t>(mStagingImage);

	vk::MemoryRequirements memoryRequirements;
	realDevice->mDevice.getImageMemoryRequirements(mStagingImage, &memoryRequirements);

	//mMemoryAllocateInfo.allocationSize = 0;
	mMemoryAllocateInfo.memoryTypeIndex = 0;
	mMemoryAllocateInfo.allocationSize = memoryRequirements.size;

	if (!GetMemoryTypeFromProperties(realDevice->mPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &mMemoryAllocateInfo.memoryTypeIndex))
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface Could not find memory type from properties.";
		return;
	}

	result = realDevice->mDevice.allocateMemory(&mMemoryAllocateInfo, nullptr, &mStagingDeviceMemory);
	if (result != vk::Result::eSuccess)
	{
		BOOST_LOG_TRIVIAL(fatal) << "RealSurface::RealSurface vkAllocateMemory failed with return code of " << GetResultString((VkResult)result);
		return;
	}

	realDevice->mDevice.bindImageMemory(mStagingImage, mStagingDeviceMemory, 0);

	mSubresource.mipLevel = 0;
	mSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	mSubresource.arrayLayer = 0; //if this is wrong you may get 4294967296.

	//BOOST_LOG_TRIVIAL(info) << "RealSurface::RealSurface (CVolume9) using format " << (VkFormat)mRealFormat;
	realDevice->mDevice.getImageSubresourceLayout(mStagingImage, &mSubresource, &mLayouts[0]);
}

RealSurface::~RealSurface()
{
	BOOST_LOG_TRIVIAL(info) << "RealSurface::~RealSurface";
	if (mRealDevice != nullptr)
	{
		auto& device = mRealDevice->mDevice;
		device.destroyImageView(mStagingImageView, nullptr);
		device.destroyImage(mStagingImage, nullptr);
		device.freeMemory(mStagingDeviceMemory, nullptr);
	}
}