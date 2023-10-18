#pragma once

#include <vulkan/vulkan.hpp>

namespace Diffuse {

	class GraphicsDevice;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Swapchain {
	public:
		Swapchain(GraphicsDevice* device);

		Swapchain() = delete;
		Swapchain(const Swapchain&) = delete;
		Swapchain(Swapchain&&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&&) = delete;

		void SetFormat(VkFormat format) { m_format = format; }
		void SetColorSpace(VkColorSpaceKHR color_space) { m_color_space = color_space; }

		VkFormat GetFormat() const { return m_format; }
		VkColorSpaceKHR GetColorSpace() const { return m_color_space; }
		VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
		std::vector<VkImage> GetSwapchainImages() const { return m_swapchain_images; }
		VkImage GetSwapchainImage(int i) const {
			if(i > -1 && i < m_swapchain_images.size())
				return m_swapchain_images[i];
			// index out of range
			assert(false);
		}
		std::vector<VkImageView> GetSwapchainImageViews() const { return m_swapchain_image_views; }
		VkImageView GetSwapchainImageView(int i) const {
			if (i > -1 && i < m_swapchain_image_views.size())
				return m_swapchain_image_views[i];
			// index out of range
			assert(false);
		}
		uint32_t GetExtentWidth() const { return m_extent.width; }
		uint32_t GetExtentHeight() const { return m_extent.height; }
		VkExtent2D GetExtent() const { return m_extent; }
		uint32_t GetImageCount() const { return m_image_count; }
		VkPresentModeKHR GetPresentMode() const { return m_present_mode; }

		void Initialize();
	private:
		VkSwapchainKHR m_swapchain;
		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		VkSurfaceFormatKHR m_surface_format;
		SwapChainSupportDetails m_swapchain_support;
		VkFormat m_format = VK_FORMAT_B8G8R8A8_SRGB;
		VkColorSpaceKHR m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		uint32_t m_image_count;
		VkExtent2D m_extent;
		VkPresentModeKHR m_present_mode;
		
		GraphicsDevice* m_device;
	};
}