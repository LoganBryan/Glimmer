#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <openxr/openxr.h>

struct SwapchainInfo
{
	XrSwapchain swapchain = XR_NULL_HANDLE;
	int64_t swapchainFormat = 0;
	std::vector<void*> imageViews;
};

template <typename SwapchainImageT, typename Traits>
class SwapchainManager
{
public:
	enum class SwapchainType : uint8_t
	{
		COLOR,
		DEPTH
	};

public:
	inline SwapchainManager(XrSession session, const std::vector<XrViewConfigurationView>& viewConfigViews) : mSession(session), mViewConfigurationViews(viewConfigViews) {}
	inline ~SwapchainManager() { DestroySwapchains(); }

	inline XrSwapchainImageBaseHeader* AllocateSwapchainImageData(XrSwapchain swapchain, SwapchainType type, uint32_t count)
	{
		swapchainImagesMap[swapchain].first = type;
		swapchainImagesMap[swapchain].second.resize(count, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
		return reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchainImagesMap[swapchain].second.data());
	}

	inline void FreeSwapchainImageData(XrSwapchain swapchain)
	{
		swapchainImagesMap[swapchain].second.clear();
		swapchainImagesMap.erase(swapchain);
	}

	inline void* GetSwapchainImage(XrSwapchain swapchain, uint32_t index) { return (void*)(uint64_t)swapchainImagesMap[swapchain].second[index].image; }

	inline const std::vector<int64_t> GetSupportedColorSwapchainFormats() {
		return Traits::GetSupportedColorSwapchainFormats();
	}

	inline const std::vector<int64_t> GetSupportedDepthSwapchainFormats() {
		return Traits::GetSupportedDepthSwapchainFormats();
	}

	inline int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& formats) {
		const std::vector<int64_t>& supportSwapchainFormats = GetSupportedColorSwapchainFormats();

		const std::vector<int64_t>::const_iterator& swapchainFormatIt = std::find_first_of(formats.begin(), formats.end(),
			std::begin(supportSwapchainFormats), std::end(supportSwapchainFormats));
		if (swapchainFormatIt == formats.end()) {
			printf("ERROR: Unable to find supported Depth Swapchain Format!");
			throw;
			return 0;
		}

		return *swapchainFormatIt;
	}

	inline int64_t SelectDepthSwapchainFormat(const std::vector<int64_t>& formats) {
		const std::vector<int64_t>& supportSwapchainFormats = GetSupportedDepthSwapchainFormats();

		const std::vector<int64_t>::const_iterator& swapchainFormatIt = std::find_first_of(formats.begin(), formats.end(),
			std::begin(supportSwapchainFormats), std::end(supportSwapchainFormats));
		if (swapchainFormatIt == formats.end()) {
			printf("ERROR: Unable to find supported Depth Swapchain Format!");
			throw;
			return 0;
		}

		return *swapchainFormatIt;
	}

	void CreateSwapchains()
	{
		// Get supported swapchain formats. Ordered by runtime preference
		uint32_t formatCount = 0;
		xrEnumerateSwapchainFormats(mSession, 0, &formatCount, nullptr);
		std::vector<int64_t> formats(formatCount);
		xrEnumerateSwapchainFormats(mSession, formatCount, &formatCount, formats.data());
		if (SelectDepthSwapchainFormat(formats) == 0)
		{
			printf("Failed to find depth format for Swapchain!");
			DEBUG_BREAK;
		}

		mColorSwapchainInfos.resize(mViewConfigurationViews.size());
		mDepthSwapchainInfos.resize(mViewConfigurationViews.size());

		for (size_t i = 0; i < mViewConfigurationViews.size(); i++)
		{
			SwapchainInfo& colorSwapchainInfo = mColorSwapchainInfos[i];
			SwapchainInfo& depthSwapchainInfo = mDepthSwapchainInfos[i];

			XrSwapchainCreateInfo swapchainCI{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
			// Color
			swapchainCI.createFlags = 0;
			swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainCI.format = SelectColorSwapchainFormat(formats);
			swapchainCI.sampleCount = mViewConfigurationViews[i].recommendedSwapchainSampleCount;
			swapchainCI.width = mViewConfigurationViews[i].recommendedImageRectWidth;
			swapchainCI.height = mViewConfigurationViews[i].recommendedImageRectHeight;
			swapchainCI.faceCount = 1;
			swapchainCI.arraySize = 1;
			swapchainCI.mipCount = 1;
			xrCreateSwapchain(mSession, &swapchainCI, &colorSwapchainInfo.swapchain);
			colorSwapchainInfo.swapchainFormat = swapchainCI.format;

			// Depth
			swapchainCI.createFlags = 0;
			swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			swapchainCI.format = SelectDepthSwapchainFormat(formats);
			swapchainCI.sampleCount = mViewConfigurationViews[i].recommendedSwapchainSampleCount;
			swapchainCI.width = mViewConfigurationViews[i].recommendedImageRectWidth;
			swapchainCI.height = mViewConfigurationViews[i].recommendedImageRectHeight;
			swapchainCI.faceCount = 1;
			swapchainCI.arraySize = 1;
			swapchainCI.mipCount = 1;
			xrCreateSwapchain(mSession, &swapchainCI, &depthSwapchainInfo.swapchain);
			depthSwapchainInfo.swapchainFormat = swapchainCI.format;

			// Get number of images in color/ depth swapchain and allocate Swapchain image data
			uint32_t colorSwapchainImageCount = 0;
			xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr);
			XrSwapchainImageBaseHeader* colorSwapchainImages = AllocateSwapchainImageData(colorSwapchainInfo.swapchain, SwapchainType::COLOR, colorSwapchainImageCount);
			xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages);

			uint32_t depthSwapchainImageCount = 0;
			xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr);
			XrSwapchainImageBaseHeader* depthSwapchainImages = AllocateSwapchainImageData(depthSwapchainInfo.swapchain, SwapchainType::DEPTH, depthSwapchainImageCount);
			xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages);

			// Per image in swapchains. 
			for (uint32_t j = 0; j < colorSwapchainImageCount; j++)
			{
				typename Traits::ImageViewCreateInfo imageViewCI;
				imageViewCI.image = GetSwapchainImage(colorSwapchainInfo.swapchain, j);
				imageViewCI.type = Traits::ImageViewCreateInfo::Type::RTV;
				imageViewCI.view = Traits::ImageViewCreateInfo::View::TYPE_2D;
				imageViewCI.format = colorSwapchainInfo.swapchainFormat;
				imageViewCI.aspect = Traits::ImageViewCreateInfo::Aspect::COLOR_BIT;
				imageViewCI.baseMipLevel = 0;
				imageViewCI.levelCount = 1;
				imageViewCI.baseArrayLayer = 0;
				imageViewCI.layerCount = 1;
				Traits traitsInstance;
				colorSwapchainInfo.imageViews.push_back(traitsInstance.CreateImageView(imageViewCI, imageViews));
			}
			for (uint32_t j = 0; j < depthSwapchainImageCount; j++)
			{
				typename Traits::ImageViewCreateInfo imageViewCI;
				imageViewCI.image = GetSwapchainImage(depthSwapchainInfo.swapchain, j);
				imageViewCI.type = Traits::ImageViewCreateInfo::Type::DSV;
				imageViewCI.view = Traits::ImageViewCreateInfo::View::TYPE_2D;
				imageViewCI.format = depthSwapchainInfo.swapchainFormat;
				imageViewCI.aspect = Traits::ImageViewCreateInfo::Aspect::DEPTH_BIT;
				imageViewCI.baseMipLevel = 0;
				imageViewCI.levelCount = 1;
				imageViewCI.baseArrayLayer = 0;
				imageViewCI.layerCount = 1;
				Traits traitsInstance;
				depthSwapchainInfo.imageViews.push_back(traitsInstance.CreateImageView(imageViewCI, imageViews));
			}
		}
	}

	void DestroySwapchains()
	{
		// Per view in view config
		for (size_t i = 0; i < mViewConfigurationViews.size(); i++)
		{
			SwapchainInfo& colorSwapchainInfo = mColorSwapchainInfos[i];
			SwapchainInfo& depthSwapchainInfo = mDepthSwapchainInfos[i];

			//Destroy color and depth image views
			for (void*& imageView : colorSwapchainInfo.imageViews)
				Traits::DestroyImageViews(imageView, imageViews);
			for (void*& imageView : depthSwapchainInfo.imageViews)
				Traits::DestroyImageViews(imageView, imageViews);

			// Free swapchain image data
			FreeSwapchainImageData(colorSwapchainInfo.swapchain);
			FreeSwapchainImageData(depthSwapchainInfo.swapchain);

			// Destroy swapchains
			xrDestroySwapchain(colorSwapchainInfo.swapchain);
			xrDestroySwapchain(depthSwapchainInfo.swapchain);
		}
	}

	inline std::vector<SwapchainInfo>& GetColorSwapchainInfo() { return mColorSwapchainInfos; }
	inline std::vector<SwapchainInfo>& GetDepthSwapchainInfo() { return mDepthSwapchainInfos; }

private:
	XrSession mSession = XR_NULL_HANDLE;
	std::vector<XrViewConfigurationView> mViewConfigurationViews;
	std::unordered_map<XrSwapchain, std::pair<SwapchainType, std::vector<SwapchainImageT>>> swapchainImagesMap{};

	std::vector<SwapchainInfo> mColorSwapchainInfos = {};
	std::vector<SwapchainInfo> mDepthSwapchainInfos = {};
	std::unordered_map<unsigned int, typename Traits::ImageViewCreateInfo> imageViews{};
};

