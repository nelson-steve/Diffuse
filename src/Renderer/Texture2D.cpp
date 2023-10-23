#include "Texture2D.hpp"

#include "GraphicsDevice.hpp"
#include "VulkanUtilities.hpp"

#include "stb_image.h"

namespace Diffuse {
	Texture2D::Texture2D(tinygltf::Image image, TextureSampler sampler, VkQueue copy_queue, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;

		unsigned char* buffer = nullptr;
		bool delete_buffer = false;
		VkDeviceSize buffer_size = 0;
		if (image.component == 3) {
			buffer_size = image.width* image.height * 4;
			buffer = new unsigned char[buffer_size];
			unsigned char* rgba = buffer;
			unsigned char* rgb = &image.image[0];
			for (int i = 0; i < image.width * image.height; i++) {
				for (int j = 0; j < 3; j++) {
					rgba[j] = rgb[j];
				}
				rgba += 4;
				rgb += 3;
			}
			delete_buffer = true;
		}
		else {
			buffer = &image.image[0];
			buffer_size = image.image.size();
		}

		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		VkFormatProperties formatProperties;

		m_width = image.width;
		m_height = image.height;
		m_mip_levels = static_cast<uint32_t>(floor(log2(std::max(m_width, m_height))) + 1.0);

		vkGetPhysicalDeviceFormatProperties(m_graphics_device->PhysicalDevice(), format, &formatProperties);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs{};

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buffer_size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(m_graphics_device->Device(), &bufferCreateInfo, nullptr, &stagingBuffer)) {
			throw std::runtime_error("failed to create buffer!");
		}
		vkGetBufferMemoryRequirements(m_graphics_device->Device(), stagingBuffer, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_graphics_device->PhysicalDevice());
		if (vkAllocateMemory(m_graphics_device->Device(), &memAllocInfo, nullptr, &stagingMemory)) {
			throw std::runtime_error("failed to load texture image!");
		}
		if (vkBindBufferMemory(m_graphics_device->Device(), stagingBuffer, stagingMemory, 0)) {
			throw std::runtime_error("failed to load texture image!");
		}

		uint8_t* data;
		if (vkMapMemory(m_graphics_device->Device(), stagingMemory, 0, memReqs.size, 0, (void**)&data)) {
			throw std::runtime_error("failed to map memory!");
		}
		memcpy(data, buffer, buffer_size);
		vkUnmapMemory(m_graphics_device->Device(), stagingMemory);

		VkImageCreateInfo image_create_info{};
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.format = format;
		image_create_info.mipLevels = m_mip_levels;
		image_create_info.arrayLayers = 1;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_create_info.extent = { m_width, m_height, 1 };
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (vkCreateImage(m_graphics_device->Device(), &image_create_info, nullptr, &m_texture_image)) {
			throw std::runtime_error("failed to create image!");
		}
		vkGetImageMemoryRequirements(m_graphics_device->Device(), m_texture_image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_graphics_device->PhysicalDevice());
		if (vkAllocateMemory(m_graphics_device->Device(), &memAllocInfo, nullptr, &m_texture_image_memory)) {
			throw std::runtime_error("failed to allocate memory!");
		}
		if (vkBindImageMemory(m_graphics_device->Device(), m_texture_image, m_texture_image_memory, 0)) {
			throw std::runtime_error("failed to find memory!");
		}

		VkCommandBuffer copy_cmd = m_graphics_device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkImageSubresourceRange subresource_range = {};
		subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.levelCount = 1;
		subresource_range.layerCount = 1;

		{
			VkImageMemoryBarrier image_memory_barrier{};
			image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			image_memory_barrier.srcAccessMask = 0;
			image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			image_memory_barrier.image = m_texture_image;
			image_memory_barrier.subresourceRange = subresource_range;
			vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
		}

		VkBufferImageCopy buffer_copy_region = {};
		buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_region.imageSubresource.mipLevel = 0;
		buffer_copy_region.imageSubresource.baseArrayLayer = 0;
		buffer_copy_region.imageSubresource.layerCount = 1;
		buffer_copy_region.imageExtent.width = m_width;
		buffer_copy_region.imageExtent.height = m_height;
		buffer_copy_region.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(copy_cmd, stagingBuffer, m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

		{
			VkImageMemoryBarrier image_memory_barrier{};
			image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.image = m_texture_image;
			image_memory_barrier.subresourceRange = subresource_range;
			vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
		}

		// TODO: implement this
		//device->flushCommandBuffer(copyCmd, copyQueue, true);
		m_graphics_device->FlushCommandBuffer(copy_cmd, copy_queue, true);

		vkFreeMemory(m_graphics_device->Device(), stagingMemory, nullptr);
		vkDestroyBuffer(m_graphics_device->Device(), stagingBuffer, nullptr);

		// Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
		VkCommandBuffer blit_cmd = m_graphics_device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		for (uint32_t i = 1; i < m_mip_levels; i++) {
			VkImageBlit imageBlit{};

			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(m_width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(m_height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(m_width >> i);
			imageBlit.dstOffsets[1].y = int32_t(m_height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = m_texture_image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blit_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			vkCmdBlitImage(blit_cmd, m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = m_texture_image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blit_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}

		subresource_range.levelCount = m_mip_levels;
		m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.image = m_texture_image;
			imageMemoryBarrier.subresourceRange = subresource_range;
			vkCmdPipelineBarrier(blit_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		m_graphics_device->FlushCommandBuffer(blit_cmd, copy_queue, true);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = sampler.mag_filter;
		samplerInfo.minFilter = sampler.min_filter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = sampler.address_modeU;
		samplerInfo.addressModeV = sampler.address_modeV;
		samplerInfo.addressModeW = sampler.address_modeW;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxLod = (float)m_mip_levels;
		samplerInfo.maxAnisotropy = 8.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		if (vkCreateSampler(m_graphics_device->Device(), &samplerInfo, nullptr, &m_texture_sampler)) {
			throw std::runtime_error("failed to create sampler!");
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_texture_image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = m_mip_levels;
		if (vkCreateImageView(m_graphics_device->Device(), &viewInfo, nullptr, &m_texture_image_view)) {
			throw std::runtime_error("failed to create image view!");
		}

		m_descriptor.sampler = m_texture_sampler;
		m_descriptor.imageView = m_texture_image_view;
		m_descriptor.imageLayout = m_imageLayout;

		if (delete_buffer)
			delete[] buffer;

	}

	Texture2D::Texture2D(const std::string& path, VkFormat format, TextureSampler sampler, VkImageUsageFlags additionalUsage, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;
		// Create Texture Image
		int texWidth, texHeight, texChannels;
		void* m_pixels;
		m_mip_levels = 1; // default
		VkDeviceSize imageSize;
		if (stbi_is_hdr(path.c_str())) {
			m_is_hdr = true;
			float* pixels = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			imageSize = (texWidth * (4 * sizeof(float))) * texHeight;
			if (!pixels) {
				throw std::runtime_error("failed to load texture image!");
			}
			m_pixels = reinterpret_cast<unsigned char*>(pixels);
		}
		else {
			bool m_is_hdr = false;
			stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			imageSize = (texWidth * (4 * sizeof(unsigned char))) * texHeight;
			if (!pixels) {
				throw std::runtime_error("failed to load texture image!");
			}
			m_pixels = pixels;
		}
		m_width = texWidth;
		m_height = texHeight;
		assert(m_width > 0 && m_height > 0);

		size_t pixel_data_size = 0;
		if (m_is_hdr) {
			pixel_data_size = (texWidth * (texChannels * sizeof(float))) * texHeight;
		}
		else {
			pixel_data_size = (texWidth * (texChannels * sizeof(unsigned char))) * texHeight;
		}

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsage;
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation

		VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		createInfo.flags = 0;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = { (unsigned int)texWidth, (unsigned int)texHeight, 1 };
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = static_cast<VkSampleCountFlagBits>(1);
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (vkCreateImage(m_graphics_device->Device(), &createInfo, nullptr, &m_texture_image) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(m_graphics_device->Device(), m_texture_image, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_graphics_device->PhysicalDevice());
		if (vkAllocateMemory(m_graphics_device->Device(), &allocateInfo, nullptr, &m_texture_image_memory)) {
			throw std::runtime_error("Failed to allocate device memory for image");
		}
		if (vkBindImageMemory(m_graphics_device->Device(), m_texture_image, m_texture_image_memory, 0) != VK_SUCCESS) {
			throw std::runtime_error("Failed to bind device memory to image");
		}

		//texture.view = createTextureView(texture, format, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
		VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewCreateInfo.image = m_texture_image;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = m_mip_levels;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_graphics_device->Device(), &viewCreateInfo, nullptr, &m_texture_image_view)) {
			throw std::runtime_error("Failed to create texture image view");
		}

		VkBuffer staging_buffer;
		VkDeviceMemory staging_memory;
		vkUtilities::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, staging_buffer,
			staging_memory, m_graphics_device->PhysicalDevice(), m_graphics_device->Device());

		// copy data to staging buffer
		const VkMappedMemoryRange flushRange = {
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			staging_memory,
			0,
			VK_WHOLE_SIZE
		};

		void* mappedMemory;
		if (vkMapMemory(m_graphics_device->Device(), staging_memory, 0, VK_WHOLE_SIZE, 0, &mappedMemory)) {
			throw std::runtime_error("Failed to map device memory to host address space");
		}
		std::memcpy(mappedMemory, m_pixels, imageSize);
		vkFlushMappedMemoryRanges(m_graphics_device->Device(), 1, &flushRange);
		vkUnmapMemory(m_graphics_device->Device(), staging_memory);

		VkCommandBuffer copy_cmd = vkUtilities::BeginSingleTimeCommands(m_graphics_device->CommandPool(), m_graphics_device->Device());
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_texture_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			uint32_t size = 1;
			vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, 0, nullptr, 0, nullptr, size, &barrier);
		}

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.imageExtent = { (unsigned int)texWidth, (unsigned int)texHeight, 1 };
		vkCmdCopyBufferToImage(copy_cmd, staging_buffer, m_texture_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		// If we're going to generate mipmaps transition base mip to transfer src layout, otherwise use shader read only layout.
		//VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // levels = 1
		VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // levels = 1
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = final_layout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_texture_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			uint32_t size = 1;
			vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, size, &barrier);
		}

		vkUtilities::EndSingleTimeCommands(copy_cmd, m_graphics_device->Device(), m_graphics_device->Queue(), m_graphics_device->CommandPool());

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = sampler.mag_filter;
		samplerInfo.minFilter = sampler.min_filter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = sampler.address_modeU;
		samplerInfo.addressModeV = sampler.address_modeV;
		samplerInfo.addressModeW = sampler.address_modeW;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxLod = (float)m_mip_levels;
		samplerInfo.maxAnisotropy = 8.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		if (vkCreateSampler(m_graphics_device->Device(), &samplerInfo, nullptr, &m_texture_sampler)) {
			throw std::runtime_error("failed to create sampler!");
		}

		m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_descriptor.sampler = m_texture_sampler;
		m_descriptor.imageView = m_texture_image_view;
		m_descriptor.imageLayout = m_imageLayout;

		if (staging_buffer != VK_NULL_HANDLE)
			vkDestroyBuffer(m_graphics_device->Device(), staging_buffer, nullptr);
		if (staging_memory != VK_NULL_HANDLE)
			vkFreeMemory(m_graphics_device->Device(), staging_memory, nullptr);

		// if level > 1
		// GenerateMipmaps()
	}

	void Texture2D::UpdateDescriptor() {
		m_descriptor.sampler = m_texture_sampler;
		m_descriptor.imageView = m_texture_image_view;
		m_descriptor.imageLayout = m_imageLayout;
	}

#if 0
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
			stagingBuffer, stagingMemory, m_graphics_device->PhysicalDevice(), m_graphics_device->Device());

		void* data;
		vkMapMemory(m_graphics_device->Device(), stagingMemory, 0, imageSize, 0, &data);
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(m_graphics_device->Device(), stagingMemory);

		vkUtilities::CreateImage(m_width, m_height, m_graphics_device->Device(), m_graphics_device->PhysicalDevice(),
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_texture_image, m_texture_image_memory, 1, 1);

		vkUtilities::TransitionImageLayout(m_graphics_device->Queue(), m_graphics_device->CommandPool(), m_graphics_device->Device(), m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkUtilities::CopyBufferToImage(m_graphics_device->Queue(), m_graphics_device->CommandPool(), m_graphics_device->Device(), stagingBuffer, m_texture_image, static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height));
		vkUtilities::TransitionImageLayout(m_graphics_device->Queue(), m_graphics_device->CommandPool(), m_graphics_device->Device(), m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_graphics_device->Device(), stagingBuffer, nullptr);
		vkFreeMemory(m_graphics_device->Device(), stagingMemory, nullptr);

		// Create Texture Image View
		m_texture_image_view = vkUtilities::CreateImageView(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_graphics_device->Device(), 1, 0, m_mipLevels);

		// Create Texture Image Sampler
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_graphics_device->PhysicalDevice(), &properties);

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

		if (vkCreateSampler(m_graphics_device->Device(), &samplerInfo, nullptr, &m_texture_sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
		}
#endif

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
		m_mip_levels = (levels > 0) ? levels : numMipmapLevels(width, height);
		m_layers = layers;

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsage;
		if (m_mip_levels > 1) {
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation
		}
		// Create Image
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.flags = (layers == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = m_mip_levels;
			imageInfo.arrayLayers = layers;
			imageInfo.format = format;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateImage(m_graphics_device->Device(), &imageInfo, nullptr, &m_texture_image) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image!");
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(m_graphics_device->Device(), m_texture_image, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex =
				vkUtilities::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_graphics_device->PhysicalDevice());

			if (vkAllocateMemory(m_graphics_device->Device(), &allocInfo, nullptr, &m_texture_image_memory) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate image memory!");
			}

			vkBindImageMemory(m_graphics_device->Device(), m_texture_image, m_texture_image_memory, 0);
		}

		// Create Texture Image View
		{
			VkImageViewCreateInfo view_info{};
			view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.image = m_texture_image;
			view_info.viewType = (layers == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.subresourceRange.baseMipLevel = 0;
			view_info.subresourceRange.levelCount = m_mip_levels;
			view_info.subresourceRange.baseArrayLayer = 0;
			view_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			VkImageView imageView;
			if (vkCreateImageView(m_graphics_device->Device(), &view_info, nullptr, &imageView) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
		}
		m_texture_image_view = vkUtilities::CreateImageView(m_texture_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_graphics_device->Device(), layers, 0, m_mip_levels);
	}

	void TextureCubemap::UpdateDescriptor() {
		m_descriptor.sampler = m_texture_sampler;
		m_descriptor.imageView = m_texture_image_view;
		m_descriptor.imageLayout = m_imageLayout;
	}
}