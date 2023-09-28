#pragma once

#include <vulkan/vulkan.hpp>

namespace Diffuse {

	class GraphicsDevice;

	class Texture2D {
	public:
		Texture2D(const std::string& path, GraphicsDevice* graphics_device);
	public:
		GraphicsDevice* m_graphics_device;

		VkImage m_texture_image;
		VkSampler m_texture_sampler;
		VkImageView m_texture_image_view;
		VkDeviceMemory m_texture_image_memory;
	};
}