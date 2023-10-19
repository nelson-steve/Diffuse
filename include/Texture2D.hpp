#pragma once

#include <vulkan/vulkan.hpp>
#include "Model.hpp"

namespace Diffuse {

	class GraphicsDevice;

    struct TextureSampler {
        VkFilter mag_filter;
        VkFilter min_filter;
        VkSamplerAddressMode address_modeU;
        VkSamplerAddressMode address_modeV;
        VkSamplerAddressMode address_modeW;
    };

	class Texture2D {
	public:
		Texture2D() {}
        Texture2D(tinygltf::Image image, TextureSampler sampler, VkQueue copy_queue, GraphicsDevice* graphics_device);
		Texture2D(const std::string& path, VkFormat format, GraphicsDevice* graphics_device);
		Texture2D(uint32_t width, uint32_t height, uint32_t layers, VkFormat format, uint32_t levels, VkImageUsageFlags additionalUsage, GraphicsDevice* graphics_device);
        void UpdateDescriptor();
	public:
		GraphicsDevice* m_graphics_device;

		uint32_t m_width, m_height, m_mip_levels;
        uint32_t m_layers;

		VkImage m_texture_image;
		VkSampler m_texture_sampler;
        VkImageLayout m_imageLayout;
		VkImageView m_texture_image_view;
		VkDeviceMemory m_texture_image_memory;
        VkDescriptorImageInfo m_descriptor;
	};

    class TextureCubemap {
    public:
        TextureCubemap() {}
        TextureCubemap(std::string filename,
            VkFormat format,
            GraphicsDevice* graphics_device,
            VkQueue copyQueue,
            VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void UpdateDescriptor();
    public:
        GraphicsDevice* m_graphics_device;

        uint32_t m_width, m_height, m_mipLevels;
        uint32_t m_layers;

        VkImage m_texture_image;
        VkSampler m_texture_sampler;
        VkImageView m_texture_image_view;
        VkDeviceMemory m_texture_image_memory;
        VkDescriptorImageInfo m_descriptor;
        VkImageLayout m_imageLayout;
    };

    struct ImageMemoryBarrier
    {
        ImageMemoryBarrier(const Texture2D& texture, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture.m_texture_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        }
        operator VkImageMemoryBarrier() const { return barrier; }
        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

        ImageMemoryBarrier& aspectMask(VkImageAspectFlags aspectMask)
        {
            barrier.subresourceRange.aspectMask = aspectMask;
            return *this;
        }
        ImageMemoryBarrier& mipLevels(uint32_t baseMipLevel, uint32_t levelCount = VK_REMAINING_MIP_LEVELS)
        {
            barrier.subresourceRange.baseMipLevel = baseMipLevel;
            barrier.subresourceRange.levelCount = levelCount;
            return *this;
        }
        ImageMemoryBarrier& arrayLayers(uint32_t baseArrayLayer, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS)
        {
            barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
            barrier.subresourceRange.layerCount = layerCount;
            return *this;
        }
    };
}