#include "Texture2D.hpp"

#include "GraphicsDevice.hpp"
#include "VulkanUtilities.hpp"

#include "ktx.h"

#include "stb_image.h"

namespace Diffuse {
	Texture2D::Texture2D(const std::string& path, VkFormat format, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;
		// Create Texture Image
		int texWidth, texHeight, texChannels;
		void* m_pixels;
		VkDeviceSize imageSize;
		if (stbi_is_hdr(path.c_str())) {
			float* pixels = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			imageSize = (texWidth * (4*sizeof(float))) * texHeight;
			if (!pixels) {
				throw std::runtime_error("failed to load texture image!");
			}
			m_pixels = reinterpret_cast<unsigned char*>(pixels);
		}
		else {
			stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			imageSize = (texWidth * (4 * sizeof(unsigned char))) * texHeight;
			if (!pixels) {
				throw std::runtime_error("failed to load texture image!");
			}
			m_pixels = pixels;
		}

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkUtilities::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

        void* data;
        vkMapMemory(m_graphics_device->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, m_pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_graphics_device->m_device, stagingBufferMemory);

        stbi_image_free(m_pixels);

        vkUtilities::CreateImage(texWidth, texHeight, m_graphics_device->m_device, m_graphics_device->m_physical_device, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory, 1, 1);

        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkUtilities::CopyBufferToImage(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, stagingBuffer, m_texture_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_graphics_device->m_device, stagingBufferMemory, nullptr);

        // Create Texture Image View
		m_texture_image_view = vkUtilities::CreateImageView(m_texture_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_graphics_device->m_device, 1, 0, 1);

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

    Texture2D::Texture2D(tinygltf::Image image, GraphicsDevice* graphics_device) {

        // Get the image data from the glTF loader
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        if (image.component == 3) {
            bufferSize = image.width * image.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &image.image[0];
            for (size_t i = 0; i < image.width * image.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        }
        else {
            buffer = &image.image[0];
            bufferSize = image.image.size();
        }

		m_graphics_device = graphics_device;
		m_width = image.width;
		m_height = image.height;
        VkDeviceSize imageSize = m_width * m_height * 4;
		m_mipLevels = 1;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		vkUtilities::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

		void* data;
		vkMapMemory(m_graphics_device->m_device, stagingMemory, 0, imageSize, 0, &data);
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(m_graphics_device->m_device, stagingMemory);

		vkUtilities::CreateImage(m_width, m_height, m_graphics_device->m_device, m_graphics_device->m_physical_device, 
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			m_texture_image, m_texture_image_memory, 1, 1);

        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkUtilities::CopyBufferToImage(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, stagingBuffer, m_texture_image, static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height));
        vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_graphics_device->m_device, stagingMemory, nullptr);

        // Create Texture Image View
		m_texture_image_view = vkUtilities::CreateImageView(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_graphics_device->m_device, 1, 0, m_mipLevels);

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

	template<typename T> static constexpr T numMipmapLevels(T width, T height)
	{
		T levels = 1;
		while ((width | height) >> levels) {
			++levels;
		}
		return levels;
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height, uint32_t layers, VkFormat format, uint32_t levels, VkImageUsageFlags additionalUsage, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;
		m_width = width;
		m_height = height;
		VkDeviceSize imageSize = m_width * m_height * 4;
		//m_mipLevels = levels;
		m_mipLevels = (levels > 0) ? levels : numMipmapLevels(width, height);

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsage;
		if (m_mipLevels > 1) {
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation
		}

		vkUtilities::CreateImage(m_width, m_height, m_graphics_device->m_device, m_graphics_device->m_physical_device, format, 
			VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory, layers, m_mipLevels);

		//vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//vkUtilities::TransitionImageLayout(m_graphics_device->m_graphics_queue, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_texture_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Create Texture Image View
		m_texture_image_view = vkUtilities::CreateImageView(m_texture_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_graphics_device->m_device, layers, 0, m_mipLevels);

		/*
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
		*/
	}

	TextureCubemap::TextureCubemap(std::string filename,
		VkFormat format,
		GraphicsDevice* graphics_device,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags,
		VkImageLayout imageLayout) {
		ktxTexture* ktxTexture;
		ktxResult result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		assert(result == KTX_SUCCESS);
		this->m_graphics_device = graphics_device;
		m_width = ktxTexture->baseWidth;
		m_height = ktxTexture->baseHeight;
		m_mipLevels = ktxTexture->numLevels;

		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		VkMemoryAllocateInfo memAllocInfo = MemoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// Create a host-visible staging buffer that contains the raw image data
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = BufferCreateInfo();
		bufferCreateInfo.size = ktxTextureSize;
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_graphics_device->Device(), &bufferCreateInfo, nullptr, &stagingBuffer)) {

		}

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vkGetBufferMemoryRequirements(graphics_device->Device(), stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, graphics_device->PhysicalDevice());

		if (vkAllocateMemory(graphics_device->Device(), &memAllocInfo, nullptr, &stagingMemory)) {

		}
		if (vkBindBufferMemory(graphics_device->Device(), stagingBuffer, stagingMemory, 0)) {

		}

		// Copy texture data into staging buffer
		uint8_t* data;
		if (vkMapMemory(graphics_device->Device(), stagingMemory, 0, memReqs.size, 0, (void**)&data)) {

		}
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(m_graphics_device->Device(), stagingMemory);

		// Setup buffer copy regions for each face including all of its mip levels
		std::vector<VkBufferImageCopy> bufferCopyRegions;

		for (uint32_t face = 0; face < 6; face++)
		{
			for (uint32_t level = 0; level < m_mipLevels; level++)
			{
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
				assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}
		}

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = ImageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = m_mipLevels;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { m_width, m_height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		// Cube faces count as array layers in Vulkan
		imageCreateInfo.arrayLayers = 6;
		// This flag is required for cube map images
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


		if (vkCreateImage(m_graphics_device->Device(), &imageCreateInfo, nullptr, &m_texture_image)) {

		}

		vkGetImageMemoryRequirements(m_graphics_device->Device(), m_texture_image, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphics_device->PhysicalDevice());

		if(vkAllocateMemory(m_graphics_device->Device(), &memAllocInfo, nullptr, &m_texture_image_memory));
		if(vkBindImageMemory(m_graphics_device->Device(), m_texture_image, m_texture_image_memory, 0));

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = CreateCommandBuffer(graphics_device->Device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, graphics_device->CommandPool(), true);

		// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = m_mipLevels;
		subresourceRange.layerCount = 6;

		SetImageLayout(
			copyCmd,
			m_texture_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy the cube map faces from the staging buffer to the optimal tiled image
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			m_texture_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data());

		// Change texture image layout to shader read after all faces have been copied
		m_imageLayout = imageLayout;
		SetImageLayout(
			copyCmd,
			m_texture_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		FlushCommandBuffer(graphics_device->Device(), copyCmd, graphics_device->Queue(), graphics_device->CommandPool(), copyQueue);

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_graphics_device->PhysicalDevice(), &properties);

		// Create sampler
		VkSamplerCreateInfo samplerCreateInfo = SamplerCreateInfo();
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = (float)m_mipLevels;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (vkCreateSampler(graphics_device->Device(), &samplerCreateInfo, nullptr, &m_texture_sampler)) {

		}

		// Create image view
		VkImageViewCreateInfo viewCreateInfo = ImageViewCreateInfo();
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.layerCount = 6;
		viewCreateInfo.subresourceRange.levelCount = m_mipLevels;
		viewCreateInfo.image = m_texture_image;
		if (vkCreateImageView(graphics_device->Device(), &viewCreateInfo, nullptr, &m_texture_image_view)) {

		}

		// Clean up staging resources
		ktxTexture_Destroy(ktxTexture);
		vkDestroyBuffer(graphics_device->Device(), stagingBuffer, nullptr);
		vkFreeMemory(graphics_device->Device(), stagingMemory, nullptr);

		// Update descriptor image info member that can be used for setting up descriptor sets
		UpdateDescriptor();
	}

	void TextureCubemap::UpdateDescriptor(){
		m_descriptor.sampler = m_texture_sampler;
		m_descriptor.imageView = m_texture_image_view;
		m_descriptor.imageLayout = m_imageLayout;
	}
}