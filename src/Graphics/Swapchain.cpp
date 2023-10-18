#include "Swapchain.hpp"

#include "GraphicsDevice.hpp"

namespace Diffuse {
	Swapchain::Swapchain(GraphicsDevice* device) {
		m_device = device;
		m_swapchain_support = vkUtilities::QuerySwapChainSupport(m_device->PhysicalDevice(), m_device->Surface());
		m_surface_format = vkUtilities::ChooseSwapSurfaceFormat(m_swapchain_support.formats);
		m_present_mode = vkUtilities::ChooseSwapPresentMode(m_swapchain_support.presentModes);
		m_extent = vkUtilities::ChooseSwapExtent(m_swapchain_support.capabilities, device->GetWindow()->window());
		m_image_count = m_swapchain_support.capabilities.minImageCount + 1;
		if (m_swapchain_support.capabilities.maxImageCount > 0 && m_image_count > m_swapchain_support.capabilities.maxImageCount) {
			m_image_count = m_swapchain_support.capabilities.maxImageCount;
		}
	}

	void Swapchain::Destroy() {
		for (auto imageView : m_swapchain_image_views)
		    vkDestroyImageView(m_device->Device(), imageView, nullptr);
		vkDestroySwapchainKHR(m_device->Device(), m_swapchain, nullptr);
	}

	void Swapchain::Initialize() {
		bool format_found = false;
		for (const auto& availableFormat : m_swapchain_support.formats) {
			if (availableFormat.format == m_format && availableFormat.colorSpace == m_color_space) {
				m_surface_format = availableFormat;
				format_found = true;
			}
		}
		if(!format_found)
		{
			// TODO: Show message -> "Preferred format isn't available using default format and color space"
			m_surface_format.format = VK_FORMAT_B8G8R8A8_SRGB;
			m_surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}

		VkSwapchainCreateInfoKHR swap_chain_create_info{};
		swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_create_info.surface = m_device->Surface();
		swap_chain_create_info.minImageCount = m_image_count;
		swap_chain_create_info.imageFormat = m_surface_format.format;
		swap_chain_create_info.imageColorSpace = m_surface_format.colorSpace;
		swap_chain_create_info.imageExtent = m_extent;
		swap_chain_create_info.imageArrayLayers = 1;
		swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices queue_family_indices = vkUtilities::FindQueueFamilies(m_device->PhysicalDevice(), m_device->Surface());
		uint32_t queueFamilyIndices[] = { queue_family_indices.graphicsFamily.value(), queue_family_indices.presentFamily.value() };
		if (queue_family_indices.graphicsFamily != queue_family_indices.presentFamily) {
			swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swap_chain_create_info.queueFamilyIndexCount = 2;
			swap_chain_create_info.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swap_chain_create_info.preTransform = m_swapchain_support.capabilities.currentTransform;
		swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_chain_create_info.presentMode = m_present_mode;
		swap_chain_create_info.clipped = VK_TRUE;
		swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;
		if (vkCreateSwapchainKHR(m_device->Device(), &swap_chain_create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Swapchain!");
		}

		vkGetSwapchainImagesKHR(m_device->Device(), m_swapchain, &m_image_count, nullptr);
		m_swapchain_images.resize(m_image_count);
		vkGetSwapchainImagesKHR(m_device->Device(), m_swapchain, &m_image_count, m_swapchain_images.data());

		// Create Image Views
		m_swapchain_image_views.resize(m_swapchain_images.size());

		for (size_t i = 0; i < m_swapchain_images.size(); i++) {
			VkImageViewCreateInfo image_views_create_info{};
			image_views_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_views_create_info.image = m_swapchain_images[i];
			image_views_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_views_create_info.format = m_format;
			image_views_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_views_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_views_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_views_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_views_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_views_create_info.subresourceRange.baseMipLevel = 0;
			image_views_create_info.subresourceRange.levelCount = 1;
			image_views_create_info.subresourceRange.baseArrayLayer = 0;
			image_views_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_device->Device(), &image_views_create_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Swapchain Image Views!");
			}
		}
	}
}