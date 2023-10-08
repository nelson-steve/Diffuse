#pragma once

#include <vulkan/vulkan.hpp>
#include "Model.hpp"

namespace Diffuse {

	class GraphicsDevice;

	class Texture2D {
	public:
		Texture2D(const std::string& path, GraphicsDevice* graphics_device);
		Texture2D(tinygltf::Image image, Model::TextureSampler texture_sampler, GraphicsDevice* graphics_device);
	public:
		GraphicsDevice* m_graphics_device;

		uint32_t m_width, m_height, m_mipLevels;

		VkImage m_texture_image;
		VkSampler m_texture_sampler;
		VkImageView m_texture_image_view;
		VkDeviceMemory m_texture_image_memory;
	};
}