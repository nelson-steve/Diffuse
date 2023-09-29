#include "Texture2D.hpp"

#include "GraphicsDevice.hpp"
#include "VulkanUtilities.hpp"

#include "stb_image.h"

namespace Diffuse {
	Texture2D::Texture2D(const std::string& path, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;

        // Create Texture Image
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkUtilities::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

        void* data;
        vkMapMemory(m_graphics_device->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_graphics_device->m_device, stagingBufferMemory);

        stbi_image_free(pixels);

        vkUtilities::CreateImage(texWidth, texHeight, m_graphics_device->m_device, m_graphics_device->m_physical_device, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory);

        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkUtilities::CopyBufferToImage(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, stagingBuffer, m_texture_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_graphics_device->m_device, stagingBufferMemory, nullptr);

        // Create Texture Image View
        m_texture_image_view = vkUtilities::CreateImageView(m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB);

        // Create Texture Image Sampler
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_graphics_device->m_physical_device, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_graphics_device->m_device, &samplerInfo, nullptr, &m_texture_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
	}

    Texture2D::Texture2D(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, GraphicsDevice* graphics_device) {
		assert(buffer);

		m_graphics_device = graphics_device;
		m_width = texWidth;
		m_height = texHeight;
        VkDeviceSize imageSize = texWidth * texHeight * 4;
		m_mipLevels = 1;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		vkUtilities::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

		void* data;
		vkMapMemory(m_graphics_device->m_device, stagingMemory, 0, imageSize, 0, &data);
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(m_graphics_device->m_device, stagingMemory);

		vkUtilities::CreateImage(texWidth, texHeight, m_graphics_device->m_device, m_graphics_device->m_physical_device, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory);

        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkUtilities::CopyBufferToImage(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, stagingBuffer, m_texture_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_graphics_device->m_device, stagingMemory, nullptr);

        // Create Texture Image View
        m_texture_image_view = vkUtilities::CreateImageView(m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB);

        // Create Texture Image Sampler
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_graphics_device->m_physical_device, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_graphics_device->m_device, &samplerInfo, nullptr, &m_texture_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

		// Update descriptor image info member that can be used for setting up descriptor sets
		//updateDescriptor();
    }
}