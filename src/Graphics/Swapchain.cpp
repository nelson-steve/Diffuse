#include "Swapchain.hpp"

#include "GraphicsDevice.hpp"

namespace Diffuse {
	Swapchain::Swapchain(GraphicsDevice* device) {
		m_device = device;
		m_swap_chain_support = vkUtilities::QuerySwapChainSupport(device->PhysicalDevice(), device->Surface());
		m_present_mode = vkUtilities::ChooseSwapPresentMode(m_swap_chain_support.presentModes);
		VkExtent2D extent = vkUtilities::ChooseSwapExtent(m_swap_chain_support.capabilities, device->GetWindow()->window());
		uint32_t m_image_count = m_swap_chain_support.capabilities.minImageCount + 1;
		if (m_swap_chain_support.capabilities.maxImageCount > 0 && m_image_count > m_swap_chain_support.capabilities.maxImageCount) {
			m_image_count = m_swap_chain_support.capabilities.maxImageCount;
		}
	}

	void Swapchain::Initialize() {
		bool format_found = false;
		for (const auto& availableFormat : m_swap_chain_support.formats) {
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

		swap_chain_create_info.preTransform = m_swap_chain_support.capabilities.currentTransform;
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
	}
}