#include "GraphicsDevice.hpp"
#include "ReadFile.hpp"
#include "Renderer.hpp"
#include "Model.hpp"
#include "Texture2D.hpp"

#include "stb_image.h"
#include "tiny_gltf.h"

#include <iostream>
#include <set>

#ifdef _DEBUG
#define LOG_ERROR(x, message) if(!x) { std::cout<<message<<std::endl; exit(1);}
#define LOG_WARN(x, message) if(!x) { std::cout<<message<<std::endl;}
#define LOG_INFO(message) { std::cout<<message<<std::endl;}
#else
#define LOG_ERROR(x, message)
#define LOG_WARN(x, message)
#define LOG_INFO(message)
#endif

namespace Diffuse {
    template<typename T> static constexpr T numMipmapLevels(T width, T height)
    {
        T levels = 1;
        while ((width | height) >> levels) {
            ++levels;
        }
        return levels;
    }

    Resource<VkImage> CreateImage(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height, uint32_t layers, uint32_t levels, VkFormat format, uint32_t samples, VkImageUsageFlags usage)
    {
        assert(width > 0);
        assert(height > 0);
        assert(levels > 0);
        assert(layers == 1 || layers == 6);
        assert(samples > 0 && samples <= 64);

        Resource<VkImage> image;

        VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        createInfo.flags = (layers == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = format;
        createInfo.extent = { width, height, 1 };
        createInfo.mipLevels = levels;
        createInfo.arrayLayers = layers;
        createInfo.samples = static_cast<VkSampleCountFlagBits>(samples);
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = usage;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(device, &createInfo, nullptr, &image.resource)) {
            throw std::runtime_error("Failed to create image");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, image.resource, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = vkUtilities::FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device);
        if (vkAllocateMemory(device, &allocateInfo, nullptr, &image.memory)) {
            throw std::runtime_error("Failed to allocate device memory for image");
        }
        if (vkBindImageMemory(device, image.resource, image.memory, 0)) {
            throw std::runtime_error("Failed to bind device memory to image");
        }

        image.allocationSize = allocateInfo.allocationSize;
        image.memoryTypeIndex = allocateInfo.memoryTypeIndex;

        return image;
    }

    RenderTarget CreateRenderTarget(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height, uint32_t samples, VkFormat colorFormat, VkFormat depthFormat) {
        assert(samples > 0 && samples <= 64);

        RenderTarget target = {};
        target.width = width;
        target.height = height;
        target.samples = samples;
        target.colorFormat = colorFormat;
        target.depthFormat = depthFormat;

        VkImageUsageFlags colorImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (samples == 1) {
            colorImageUsage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        if (colorFormat != VK_FORMAT_UNDEFINED) {
            target.colorImage = CreateImage(device, physical_device, width, height, 1, 1, colorFormat, samples, colorImageUsage);
        }
        if (depthFormat != VK_FORMAT_UNDEFINED) {
            target.depthImage = CreateImage(device, physical_device, width, height, 1, 1, depthFormat, samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }

        VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.layerCount = 1;

        if (target.colorImage.resource != VK_NULL_HANDLE) {
            viewCreateInfo.image = target.colorImage.resource;
            viewCreateInfo.format = colorFormat;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            if (vkCreateImageView(device, &viewCreateInfo, nullptr, &target.colorView)) {
                throw std::runtime_error("Failed to create render target color image view");
            }
        }

        if (target.depthImage.resource != VK_NULL_HANDLE) {
            viewCreateInfo.image = target.depthImage.resource;
            viewCreateInfo.format = depthFormat;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (vkCreateImageView(device, &viewCreateInfo, nullptr, &target.depthView)) {
                throw std::runtime_error("Failed to create render target depth-stencil image view");
            }
        }

        return target;
    }

    uint32_t QueryRenderTargetFormatMaxSamples(VkPhysicalDevice device, VkFormat format, VkImageUsageFlags usage)
    {
        VkImageFormatProperties properties;
        if (vkGetPhysicalDeviceImageFormatProperties(device, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &properties)) {
            return 0;
        }

        for (VkSampleCountFlags maxSampleCount = VK_SAMPLE_COUNT_64_BIT; maxSampleCount > VK_SAMPLE_COUNT_1_BIT; maxSampleCount >>= 1) {
            if (properties.sampleCounts & maxSampleCount) {
                return static_cast<uint32_t>(maxSampleCount);
            }
        }
        return 1;
    }

    VkImageView CreateTextureView(VkDevice device, const Texture2D& texture, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t numMipLevels)
    {
        VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCreateInfo.image = texture.m_texture_image;
        viewCreateInfo.viewType = (texture.m_layers == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.subresourceRange.aspectMask = aspectMask;
        viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
        viewCreateInfo.subresourceRange.levelCount = numMipLevels;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkImageView view;
        if (vkCreateImageView(device, &viewCreateInfo, nullptr, &view)) {
            throw std::runtime_error("Failed to create texture image view");
        }
        return view;
    }

    VkCommandBuffer BeginImmediateCommandBuffer(VkCommandBuffer commandbuffer)
    {
        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(commandbuffer, &beginInfo)) {
            throw std::runtime_error("Failed to begin immediate command buffer (still in recording state?)");
        }
        return commandbuffer;
    }

    void ExecuteImmediateCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
    {
        if (vkEndCommandBuffer(commandBuffer)) {
            throw std::runtime_error("Failed to end immediate command buffer");
        }

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        if (vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)) {
            throw std::runtime_error("Failed to reset immediate command buffer");
        }
    }

    void PipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const std::vector<ImageMemoryBarrier>& barriers)
    {
        vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), reinterpret_cast<const VkImageMemoryBarrier*>(barriers.data()));
    }

    VkPipeline CreateComputePipeline(VkDevice device, const std::string& cs, VkPipelineLayout layout,
        const VkSpecializationInfo* specializationInfo = nullptr)
    {
        auto compute_shader_code = Utils::File::ReadFile(cs);

        VkShaderModule computeShader = vkUtilities::CreateShaderModule(compute_shader_code, device);

        const VkPipelineShaderStageCreateInfo shaderStage = {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, computeShader, "main", specializationInfo,
        };

        VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        createInfo.stage = shaderStage;
        createInfo.layout = layout;

        VkPipeline pipeline;
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline)) {
            throw std::runtime_error("Failed to create compute pipeline");
        }

        vkDestroyShaderModule(device, computeShader, nullptr);

        return pipeline;
    }

    void UpdateDescriptorSet(VkDevice device, VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo>& descriptors)
    {
        VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.dstBinding = dstBinding;
        writeDescriptorSet.descriptorType = descriptorType;
        writeDescriptorSet.descriptorCount = (uint32_t)descriptors.size();
        writeDescriptorSet.pImageInfo = descriptors.data();
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

    VkPipelineLayout CreatePipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayout>* setLayouts, const std::vector<VkPushConstantRange>* pushConstants)
    {
        VkPipelineLayout layout;
        VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        if (setLayouts && setLayouts->size() > 0) {
            createInfo.setLayoutCount = (uint32_t)setLayouts->size();
            createInfo.pSetLayouts = setLayouts->data();
        }
        if (pushConstants && pushConstants->size() > 0) {
            createInfo.pushConstantRangeCount = (uint32_t)pushConstants->size();
            createInfo.pPushConstantRanges = pushConstants->data();
        }
        if (vkCreatePipelineLayout(device, &createInfo, nullptr, &layout)) {
            throw std::runtime_error("Failed to create pipeline layout");
        }
        return layout;
    }

    VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
    {
        VkDescriptorSet descriptorSet;
        VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocateInfo.descriptorPool = pool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &layout;
        if (vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet)) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }
        return descriptorSet;
    }

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>* bindings)
    {
        VkDescriptorSetLayout layout;
        VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        if (bindings && bindings->size() > 0) {
            createInfo.bindingCount = (uint32_t)bindings->size();
            createInfo.pBindings = bindings->data();
        }
        if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout)) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }
        return layout;
    }

    GraphicsDevice::GraphicsDevice(Config config) {
        // === Initializing GLFW ===
        int result = glfwInit();
        LOG_ERROR(result == GLFW_TRUE, "Failed to intitialize GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window = std::make_unique<Window>();
        glfwSetWindowUserPointer(m_window->window(), this);
        glfwSetFramebufferSizeCallback(m_window->window(), vkUtilities::FramebufferResizeCallback);
        // ============================

        // === Create Vulkan Instancee ===
        //
        //if (enable_validation_layers && !vkUtilities::CheckValidationLayerSupport()) {
        //	std::cout << "validation layers requested, but not available!";
        //	assert(false);
        //}

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Diffuse Vulkan Renderer";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Diffuse";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        std::vector<const char*> extensions = vkUtilities::GetRequiredExtensions(false); // TODO: add a boolean for if validation layers is enabled

        // TODO: Use multiple validation layers as a backup 
        const std::vector<const char*> validation_layers = {
            "VK_LAYER_KHRONOS_validation",
        };

        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames = extensions.data();
        instance_create_info.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
        instance_create_info.ppEnabledLayerNames = config.validation_layers.data();

        if (config.enable_validation_layers) {
            VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
            debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            //debug_create_info.pfnUserCallback = vkUtilities::DebugCallback;
            //vkUtilities::PopulateDebugMessengerCreateInfo(debug_create_info);
            instance_create_info.pNext = &debug_create_info;
        }

        if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create Vulkan instance!");
        }
        // ===============================        
        // --Setup Debug Messenger--
        if (config.enable_validation_layers) {
            VkDebugUtilsMessengerCreateInfoEXT messenger_create_info;
            vkUtilities::PopulateDebugMessengerCreateInfo(messenger_create_info);

            if (vkUtilities::CreateDebugUtilsMessengerEXT(m_instance, &messenger_create_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
                LOG_WARN(false, "**Failed to set up debug messenger**");
            }
        }
        // --Create Surface--
        // TODO: Use VkCreateWin32SurfaceKHR instead
        if (glfwCreateWindowSurface(m_instance, m_window->window(), nullptr, &m_surface) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create window surface!");
        }

        // --Pick Physical Device--
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

        if (device_count == 0) {
            LOG_ERROR(false, "failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

        for (const auto& device : devices) {
            if (vkUtilities::IsDeviceSuitable(device, m_surface, config.required_device_extensions)) {
                m_physical_device = device;
                break;
            }
        }

        if (m_physical_device == VK_NULL_HANDLE) {
            LOG_ERROR(false, "Failed to find a suitable GPU!");
        }

        //		--Create Logical Device--
        QueueFamilyIndices indices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        float queue_priority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queueFamily;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;
        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.pEnabledFeatures = &device_features;
        device_create_info.enabledExtensionCount = 0;
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(config.required_device_extensions.size());
        device_create_info.ppEnabledExtensionNames = config.required_device_extensions.data();
        if (config.enable_validation_layers) {
            device_create_info.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
            device_create_info.ppEnabledLayerNames = config.validation_layers.data();
        }
        else {
            device_create_info.enabledLayerCount = 0;
        }
        if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create logical device!");
        }
        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_present_queue);

        //		--Create Swap Chain--
        SwapChainSupportDetails swap_chain_support = vkUtilities::QuerySwapChainSupport(m_physical_device, m_surface);
        VkSurfaceFormatKHR surfaceFormat = vkUtilities::ChooseSwapSurfaceFormat(swap_chain_support.formats);
        VkPresentModeKHR presentMode = vkUtilities::ChooseSwapPresentMode(swap_chain_support.presentModes);
        VkExtent2D extent = vkUtilities::ChooseSwapExtent(swap_chain_support.capabilities, m_window->window());
        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 2;
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR swap_chain_create_info{};
        swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_chain_create_info.surface = m_surface;
        swap_chain_create_info.minImageCount = image_count;
        swap_chain_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swap_chain_create_info.imageColorSpace = surfaceFormat.colorSpace;
        swap_chain_create_info.imageExtent = extent;
        swap_chain_create_info.imageArrayLayers = 1;
        swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        QueueFamilyIndices queue_familt_indices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);
        uint32_t queueFamilyIndices[] = { queue_familt_indices.graphicsFamily.value(), queue_familt_indices.presentFamily.value() };
        if (queue_familt_indices.graphicsFamily != queue_familt_indices.presentFamily) {
            swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swap_chain_create_info.queueFamilyIndexCount = 2;
            swap_chain_create_info.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        swap_chain_create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swap_chain_create_info.presentMode = presentMode;
        swap_chain_create_info.clipped = VK_TRUE;
        swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(m_device, &swap_chain_create_info, nullptr, &m_swap_chain) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
        m_swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

        m_swap_chain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
        m_swap_chain_extent = extent;

        // Create Image Views
        m_swap_chain_image_views.resize(m_swap_chain_images.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
            VkImageViewCreateInfo image_views_create_info{};
            image_views_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_views_create_info.image = m_swap_chain_images[i];
            image_views_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_views_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
            image_views_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_views_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_views_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_views_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_views_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_views_create_info.subresourceRange.baseMipLevel = 0;
            image_views_create_info.subresourceRange.levelCount = 1;
            image_views_create_info.subresourceRange.baseArrayLayer = 0;
            image_views_create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &image_views_create_info, nullptr, &m_swap_chain_image_views[i]) != VK_SUCCESS) {
                LOG_ERROR(false, "Failed to create image views!");
            }
        }

        // Create render targets
        {
            uint32_t maxSamples = 4;
            const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
            const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

            const uint32_t maxColorSamples = QueryRenderTargetFormatMaxSamples(m_physical_device, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            const uint32_t maxDepthSamples = QueryRenderTargetFormatMaxSamples(m_physical_device, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

            m_renderSamples = std::min({ uint32_t(maxSamples), maxColorSamples, maxDepthSamples });
            assert(m_renderSamples >= 1);
            m_renderSamples = 1;
            m_renderTargets.resize(m_numFrames);
            //m_resolveRenderTargets.resize(m_numFrames);
            for (uint32_t i = 0; i < m_numFrames; ++i) {
                m_renderTargets[i] = CreateRenderTarget(m_device, m_physical_device, m_swap_chain_extent.width, m_swap_chain_extent.height, m_renderSamples, colorFormat, depthFormat);
                if (m_renderSamples > 1) {
                    assert(false);
                    //m_resolveRenderTargets[i] = createRenderTarget(m_device, m_physical_device, width, height, 1, colorFormat, VK_FORMAT_UNDEFINED);
                }
            }
        }
#if 0
        // Create Render Pass
        VkAttachmentDescription color_attachment{};
        color_attachment.format = m_swap_chain_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth_attachment{};
        depth_attachment.format = vkUtilities::FindDepthFormat(m_physical_device);
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create render pass!");
        }
#endif
        // Create Command Pool
        //QueueFamilyIndices queueFamilyIndices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = indices.graphicsFamily.value();

        if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create command pool!");
        }

        // Create Depth Resource

        VkFormat depthFormat = vkUtilities::FindDepthFormat(m_physical_device);

        vkUtilities::CreateImage(m_swap_chain_extent.width, m_swap_chain_extent.height, m_device, m_physical_device, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depth_image, m_depth_image_memory, 1, 1);
        m_depth_image_view = vkUtilities::CreateImageView(m_depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_device, 1, 0, 1);

        // Create Uniform Buffers
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniform_buffers[i], m_uniform_buffers_memory[i], m_physical_device, m_device);

            vkMapMemory(m_device, m_uniform_buffers_memory[i], 0, bufferSize, 0, &m_uniform_buffers_mapped[i]);
        }

        // Create Framebuffers
#if 0
        m_framebuffers.resize(m_swap_chain_image_views.size());

        for (size_t i = 0; i < m_swap_chain_image_views.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                m_swap_chain_image_views[i],
                m_depth_image_view
            };

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = m_render_pass;
            framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = m_swap_chain_extent.width;
            framebuffer_info.height = m_swap_chain_extent.height;
            framebuffer_info.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
                LOG_ERROR(false, "Failed to create framebuffer!");
            }
        }
#endif

        // Create Command Buffers
        m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = (uint32_t)m_command_buffers.size();

        if (vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to allocate command buffers!");
        }

        // Create Sync Obects
        m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
                LOG_ERROR(false, "Failed to create synchronization objects for a frame!");
            }
        }

        const std::array<VkDescriptorPoolSize, 3> poolSizes = {{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 16 },
		}};

		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = 16;
		createInfo.poolSizeCount = (uint32_t)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		if(vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_descriptor_pool)) {
			throw std::runtime_error("Failed to create descriptor pool");
		}

        // SUCCESS
    }

    VkPipeline GraphicsDevice::CreateGraphicsPipeline(uint32_t subpass,
        const std::string& vs, const std::string& fs, VkPipelineLayout layout,
        const std::vector<VkVertexInputBindingDescription>* vertexInputBindings,
        const std::vector<VkVertexInputAttributeDescription>* vertexAttributes,
        const VkPipelineMultisampleStateCreateInfo* multisampleState,
        const VkPipelineDepthStencilStateCreateInfo* depthStencilState)
    {
        const VkViewport defaultViewport = {
            0.0f,
            0.0f,
            (float)m_swap_chain_extent.width,
            (float)m_swap_chain_extent.height,
            0.0f,
            1.0f
        };
        const VkPipelineMultisampleStateCreateInfo defaultMultisampleState = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr,
            0,
            VK_SAMPLE_COUNT_1_BIT,
        };

        VkPipelineColorBlendAttachmentState defaultColorBlendAttachmentState = {};
        defaultColorBlendAttachmentState.blendEnable = VK_FALSE;
        defaultColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        auto vert_shader_code = Utils::File::ReadFile(vs);
        auto frag_shader_code = Utils::File::ReadFile(fs);

        VkShaderModule vertexShader = vkUtilities::CreateShaderModule(vert_shader_code, m_device);
        VkShaderModule fragmentShader = vkUtilities::CreateShaderModule(frag_shader_code, m_device);

        const VkPipelineShaderStageCreateInfo shaderStages[] = {
            { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   vertexShader, "main", nullptr },
            { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main", nullptr },
        };

        VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        if (vertexInputBindings) {
            vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexInputBindings->size();
            vertexInputState.pVertexBindingDescriptions = vertexInputBindings->data();
        }
        if (vertexAttributes) {
            vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributes->size();
            vertexInputState.pVertexAttributeDescriptions = vertexAttributes->data();
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;

        m_frame_rect = { 0, 0, (uint32_t)m_swap_chain_extent.width, (uint32_t)m_swap_chain_extent.height };
        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        viewportState.pViewports = &defaultViewport;
        viewportState.pScissors = &m_frame_rect;

        VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.lineWidth = 1.0f;

        const VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
            defaultColorBlendAttachmentState
        };
        VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = colorBlendAttachmentStates;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        pipelineCreateInfo.pVertexInputState = &vertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pMultisampleState = (multisampleState != nullptr) ? multisampleState : &defaultMultisampleState;
        pipelineCreateInfo.pDepthStencilState = depthStencilState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.renderPass = m_render_pass;
        pipelineCreateInfo.subpass = subpass;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline)) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(m_device, vertexShader, nullptr);
        vkDestroyShaderModule(m_device, fragmentShader, nullptr);

        return pipeline;
    }

    void GraphicsDevice::CreateVertexBuffer(VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory, const std::vector<Vertex> vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, m_physical_device, m_device);

        void* data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device, stagingBufferMemory);

        vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory, m_physical_device, m_device);

        vkUtilities::CopyBuffer(stagingBuffer, vertex_buffer, bufferSize, m_command_pool, m_device, m_graphics_queue);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }
    void GraphicsDevice::CreateIndexBuffer(VkBuffer& index_buffer, VkDeviceMemory& index_buffer_memory, const std::vector<uint32_t> indices) {
        //m_indices_size = indices.size();
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, m_physical_device, m_device);

        void* data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device, stagingBufferMemory);

        vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory, m_physical_device, m_device);

        vkUtilities::CopyBuffer(stagingBuffer, index_buffer, bufferSize, m_command_pool, m_device, m_graphics_queue);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    void GraphicsDevice::Setup(Model& model) {
        // Parameters
        static constexpr uint32_t kEnvMapSize = 1024;
        static constexpr uint32_t kIrradianceMapSize = 32;
        static constexpr uint32_t kBRDF_LUT_Size = 256;
        static constexpr uint32_t kEnvMapLevels = numMipmapLevels(kEnvMapSize, kEnvMapSize);
        static constexpr VkDeviceSize kUniformBufferSize = 64 * 1024;

        // Common descriptor set layouts
        struct {
            VkDescriptorSetLayout uniforms;
            VkDescriptorSetLayout pbr;
            VkDescriptorSetLayout skybox;
            VkDescriptorSetLayout tonemap;
            VkDescriptorSetLayout compute;
        } setLayout;

        // Friendly binding names for per-frame uniform blocks
        enum UniformsDescriptorSetBindingNames : uint32_t {
            Binding_TransformUniforms = 0,
            Binding_ShadingUniforms = 1,
        };

        // Friendly binding names for compute pipeline descriptor set
        enum ComputeDescriptorSetBindingNames : uint32_t {
            Binding_InputTexture = 0,
            Binding_OutputTexture = 1,
            Binding_OutputMipTail = 2,
        };

        // Create host-mapped uniform buffer for sub-allocation of uniform block ranges.
        //m_uniformBuffer = createUniformBuffer(kUniformBufferSize);

        // Create samplers.
        {
            VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

            // Linear, non-anisotropic sampler, wrap address mode (post processing compute shaders)
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            if (vkCreateSampler(m_device, &createInfo, nullptr, &m_compute_sampler)) {
                throw std::runtime_error("Failed to create pre-processing sampler");
            }

            // Create Texture Image Sampler
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(m_physical_device, &properties);

            // Linear, anisotropic sampler, wrap address mode (rendering)
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.anisotropyEnable = VK_TRUE;
            createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            createInfo.minLod = 0.0f;
            createInfo.maxLod = FLT_MAX;
            if (vkCreateSampler(m_device, &createInfo, nullptr, &m_default_sampler)) {
                throw std::runtime_error("Failed to create default anisotropic sampler");
            }

            // Linear, non-anisotropic sampler, clamp address mode (sampling BRDF LUT)
            createInfo.anisotropyEnable = VK_FALSE;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            if (vkCreateSampler(m_device, &createInfo, nullptr, &m_brdf_sampler)) {
                throw std::runtime_error("Failed to create BRDF LUT sampler");
            }
        }

        // Create temporary descriptor pool for pre-processing compute shaders.
        VkDescriptorPool computeDescriptorPool;
        {
            const std::array<VkDescriptorPoolSize, 2> poolSizes = { {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kEnvMapLevels },
            } };

            VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            createInfo.maxSets = 2;
            createInfo.poolSizeCount = (uint32_t)poolSizes.size();
            createInfo.pPoolSizes = poolSizes.data();
            if (vkCreateDescriptorPool(m_device, &createInfo, nullptr, &computeDescriptorPool)) {
                throw std::runtime_error("Failed to create setup descriptor pool");
            }
        }

        // Create common descriptor set & pipeline layout for pre-processing compute shaders.
        VkPipelineLayout computePipelineLayout;
        VkDescriptorSet computeDescriptorSet;
        {
            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                { Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, &m_compute_sampler },
                { Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
                { Binding_OutputMipTail, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kEnvMapLevels - 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
            };

            setLayout.compute = CreateDescriptorSetLayout(m_device, &descriptorSetLayoutBindings);
            computeDescriptorSet = AllocateDescriptorSet(m_device, computeDescriptorPool, setLayout.compute);

            const std::vector<VkDescriptorSetLayout> pipelineSetLayouts = {
                setLayout.compute,
            };
            const std::vector<VkPushConstantRange> pipelinePushConstantRanges = {
                { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants) },
            };
            computePipelineLayout = CreatePipelineLayout(m_device, &pipelineSetLayouts, &pipelinePushConstantRanges);
        }

        // Create descriptor set layout for per-frame shader uniforms
        {
            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                { Binding_TransformUniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr },
                { Binding_ShadingUniforms,   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            };
            setLayout.uniforms = CreateDescriptorSetLayout(m_device, &descriptorSetLayoutBindings);
        }

        // Allocate & update per-frame uniform buffer descriptor sets
        //{
        //    m_uniformsDescriptorSets.resize(m_numFrames);
        //    for (uint32_t i = 0; i < m_numFrames; ++i) {
        //        m_uniformsDescriptorSets[i] = AllocateDescriptorSet(m_descriptor_pool, setLayout.uniforms);
        //
        //        // Sub-allocate storage for uniform blocks
        //        m_transformUniforms.push_back(allocFromUniformBuffer<TransformUniforms>(m_uniformBuffer));
        //        UpdateDescriptorSet(m_device, m_uniformsDescriptorSets[i], Binding_TransformUniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { m_transformUniforms[i].descriptorInfo });
        //
        //        m_shadingUniforms.push_back(allocFromUniformBuffer<ShadingUniforms>(m_uniformBuffer));
        //        updateDescriptorSet(m_uniformsDescriptorSets[i], Binding_ShadingUniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { m_shadingUniforms[i].descriptorInfo });
        //    }
        //}

        // Create render pass
        {
            enum AttachmentName : uint32_t {
                MainColorAttachment = 0,
                MainDepthStencilAttachment,
                SwapchainColorAttachment,
                ResolveColorAttachment,
            };

            std::vector<VkAttachmentDescription> attachments = {
                // Main color attachment (0)
                {
                    0,
                    m_renderTargets[0].colorFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                },
                // Main depth-stencil attachment (1)
                {
                    0,
                    m_renderTargets[0].depthFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                },
                // Swapchain color attachment (2)
                {
                    0,
                    VK_FORMAT_B8G8R8A8_UNORM,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                },
            };
            //if (m_renderSamples > 1) {
            //    // Resolve color attachment (3)
            //    const VkAttachmentDescription resolveAttachment =
            //    {
            //        0,
            //        m_resolveRenderTargets[0].colorFormat,
            //        VK_SAMPLE_COUNT_1_BIT,
            //        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            //        VK_ATTACHMENT_STORE_OP_DONT_CARE,
            //        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            //        VK_ATTACHMENT_STORE_OP_DONT_CARE,
            //        VK_IMAGE_LAYOUT_UNDEFINED,
            //        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            //    };
            //    attachments.push_back(resolveAttachment);
            //}

            // Main subpass
            const std::array<VkAttachmentReference, 1> mainPassColorRefs = {
                { MainColorAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
            };
            const std::array<VkAttachmentReference, 1> mainPassResolveRefs = {
                { ResolveColorAttachment, VK_IMAGE_LAYOUT_GENERAL },
            };
            const VkAttachmentReference mainPassDepthStencilRef = {
                  MainDepthStencilAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            };
            VkSubpassDescription mainPass = {};
            mainPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            mainPass.colorAttachmentCount = (uint32_t)mainPassColorRefs.size();
            mainPass.pColorAttachments = mainPassColorRefs.data();
            mainPass.pDepthStencilAttachment = &mainPassDepthStencilRef;
            //if (m_renderSamples > 1) {
            //    mainPass.pResolveAttachments = mainPassResolveRefs.data();
            //}

            // Tonemapping subpass
            const std::array<VkAttachmentReference, 1> tonemapPassInputRefs = {
                { MainColorAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            };
            const std::array<VkAttachmentReference, 1> tonemapPassMultisampleInputRefs = {
                { ResolveColorAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            };
            const std::array<VkAttachmentReference, 1> tonemapPassColorRefs = {
                { SwapchainColorAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
            };
            VkSubpassDescription tonemapPass = {};
            tonemapPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            tonemapPass.colorAttachmentCount = (uint32_t)tonemapPassColorRefs.size();
            tonemapPass.pColorAttachments = tonemapPassColorRefs.data();
            //if (m_renderSamples > 1) {
            //    tonemapPass.inputAttachmentCount = (uint32_t)tonemapPassMultisampleInputRefs.size();
            //    tonemapPass.pInputAttachments = tonemapPassMultisampleInputRefs.data();
            //}
            //else 
            {
                tonemapPass.inputAttachmentCount = (uint32_t)tonemapPassInputRefs.size();
                tonemapPass.pInputAttachments = tonemapPassInputRefs.data();
            }

            const std::array<VkSubpassDescription, 2> subpasses = {
                mainPass,
                tonemapPass,
            };

            // Main->Tonemapping dependency
            const VkSubpassDependency mainToTonemapDependency = {
                0,
                1,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
            };

            VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
            createInfo.attachmentCount = (uint32_t)attachments.size();
            createInfo.pAttachments = attachments.data();
            createInfo.subpassCount = (uint32_t)subpasses.size();
            createInfo.pSubpasses = subpasses.data();
            createInfo.dependencyCount = 1;
            createInfo.pDependencies = &mainToTonemapDependency;

            if (vkCreateRenderPass(m_device, &createInfo, nullptr, &m_render_pass)) {
                throw std::runtime_error("Failed to create render pass");
            }
    }

        // Create framebuffers
        {
            m_framebuffers.resize(m_numFrames);
            for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {

                std::vector<VkImageView> attachments = {
                    m_renderTargets[i].colorView,
                    m_renderTargets[i].depthView,
                    m_swap_chain_image_views[i],
                };
                //if (m_renderSamples > 1) {
                //    attachments.push_back(m_resolveRenderTargets[i].colorView);
                //}

                VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
                createInfo.renderPass = m_render_pass;
                createInfo.attachmentCount = (uint32_t)attachments.size();
                createInfo.pAttachments = attachments.data();
                createInfo.width = m_swap_chain_extent.width;
                createInfo.height = m_swap_chain_extent.height;
                createInfo.layers = 1;

                if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_framebuffers[i])) {
                    throw std::runtime_error("Failed to create framebuffer");
                }
            }
        }

        // Allocate common textures for later processing.
        {
            // Environment map (with pre-filtered mip chain)
            m_envTexture = new Texture2D(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 0, VK_IMAGE_USAGE_STORAGE_BIT, this);
            // Irradiance map
            m_irmapTexture = new Texture2D(kIrradianceMapSize, kIrradianceMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 1, VK_IMAGE_USAGE_STORAGE_BIT, this);
            // 2D LUT for split-sum approximation
            m_spBRDF_LUT = new Texture2D(kBRDF_LUT_Size, kBRDF_LUT_Size, 1, VK_FORMAT_R16G16_SFLOAT, 1, VK_IMAGE_USAGE_STORAGE_BIT, this);
        }

        // Create graphics pipeline & descriptor set layout for tone mapping
        {
            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                { 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            };
            setLayout.tonemap = CreateDescriptorSetLayout(m_device, &descriptorSetLayoutBindings);

            const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
                setLayout.tonemap,
            };
            m_pipelinelayouts.tonemap = CreatePipelineLayout(m_device, &pipelineDescriptorSetLayouts, nullptr);
            m_pipelines.tonemap = CreateGraphicsPipeline(
                1,
                "../shaders/tonemap/vert.spv",
                "../shaders/tonemap/frag.spv",
                m_pipelinelayouts.tonemap);
        }

        // Allocate & update descriptor sets for tone mapping input (per-frame)
        {
            m_descriptorsets.tonemap.resize(m_numFrames);
            for (uint32_t i = 0; i < m_numFrames; ++i) {
                const VkDescriptorImageInfo imageInfo = {
                    VK_NULL_HANDLE,
                    //(m_renderSamples > 1) ? m_resolveRenderTargets[i].colorView : m_renderTargets[i].colorView,
                    m_renderTargets[i].colorView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                m_descriptorsets.tonemap[i] = AllocateDescriptorSet(m_device, m_descriptor_pool, setLayout.tonemap);
                UpdateDescriptorSet(m_device, m_descriptorsets.tonemap[i], 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, { imageInfo });
            }
        }

        // Load PBR model assets.
        //m_pbrModel = createMeshBuffer(Mesh::fromFile("meshes/cerberus.fbx"));
        //
        m_albedoTexture = new Texture2D("../assets/damaged_helmet/Default_albedo.jpg", VK_FORMAT_R8G8B8A8_SRGB, this);
        m_normalTexture = new Texture2D("../assets/damaged_helmet/Default_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM, this);
        m_metalnessTexture = new Texture2D("../assets/damaged_helmet/Default_metalRoughness.jpg", VK_FORMAT_R8_UNORM, this);
        m_roughnessTexture = new Texture2D("../assets/damaged_helmet/Default_metalRoughness.jpg", VK_FORMAT_R8_UNORM, this);

        // Create graphics pipeline & descriptor set layout for rendering PBR model
        {
            const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
            };
            const std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
                { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0  }, // Position
                { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 }, // Normal
                { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24 }, // Tangent
                { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, 36 }, // Bitangent
                { 4, 0, VK_FORMAT_R32G32_SFLOAT,    48 }, // Texcoord
            };

            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Albedo texture
                { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Normal texture
                { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Metalness texture
                { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Roughness texture
                { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Specular env map texture
                { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler }, // Irradiance map texture
                { 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_brdf_sampler },  // Specular BRDF LUT
            };
            setLayout.pbr = CreateDescriptorSetLayout(m_device, &descriptorSetLayoutBindings);

            const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
                setLayout.uniforms,
                setLayout.pbr,
            };
            m_pipelinelayouts.pbr = CreatePipelineLayout(m_device, &pipelineDescriptorSetLayouts, nullptr);

            VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(m_renderTargets[0].samples);

            VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            depthStencilState.depthTestEnable = VK_TRUE;
            depthStencilState.depthWriteEnable = VK_TRUE;
            depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

            m_pipelines.pbr = CreateGraphicsPipeline(
                0,
                "../shaders/pbr/pbr_vs.spv",
                "../shaders/pbr/pbr_fs.spv",
                m_pipelinelayouts.pbr,
                &vertexInputBindings,
                &vertexAttributes,
                &multisampleState,
                &depthStencilState);
        }

        // Allocate & update descriptor set for PBR model
        {
            const std::vector<VkDescriptorImageInfo> textures = {
                { VK_NULL_HANDLE, m_albedoTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_normalTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_metalnessTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_roughnessTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_envTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_irmapTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                { VK_NULL_HANDLE, m_spBRDF_LUT->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            };
            m_descriptorsets.pbr = AllocateDescriptorSet(m_device, m_descriptor_pool, setLayout.pbr);
            UpdateDescriptorSet(m_device, m_descriptorsets.pbr, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textures);
        }

        // Load skybox assets.
        //m_skybox = createMeshBuffer(Mesh::fromFile("meshes/skybox.obj"));

        // Create graphics pipeline & descriptor set layout for skybox
        {
            const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
            };
            const std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
                { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // Position
            };

            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_default_sampler}, // Environment texture
            };
            setLayout.skybox = CreateDescriptorSetLayout(m_device, &descriptorSetLayoutBindings);

            const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
                setLayout.uniforms,
                setLayout.skybox,
            };
            m_pipelinelayouts.skybox = CreatePipelineLayout(m_device, &pipelineDescriptorSetLayouts, nullptr);

            VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(m_renderTargets[0].samples);

            VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            depthStencilState.depthTestEnable = VK_FALSE;

            m_pipelines.skybox = CreateGraphicsPipeline(0,
                "../shaders/skybox/vert.spv",
                "../shaders/skybox/frag.spv",
                m_pipelinelayouts.skybox,
                &vertexInputBindings,
                &vertexAttributes,
                &multisampleState,
                &depthStencilState);
        }

        // Allocate & update descriptor set for skybox.
        {
            const VkDescriptorImageInfo skyboxTexture = { VK_NULL_HANDLE, m_envTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            m_descriptorsets.skybox = AllocateDescriptorSet(m_device, m_descriptor_pool, setLayout.skybox);
            UpdateDescriptorSet(m_device, m_descriptorsets.skybox, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { skyboxTexture });
        }

        // Load & pre-process environment map.
        {
            Texture2D envTextureUnfiltered(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 0, VK_IMAGE_USAGE_STORAGE_BIT, this);

            // Load & convert equirectangular envuronment map to cubemap texture
            {
                VkPipeline pipeline = CreateComputePipeline(m_device, "../shaders/equirect2cube.spv", computePipelineLayout);

                //Texture2D* envTextureEquirect;
                Texture2D envTextureEquirect("../assets/environment.hdr", VK_FORMAT_R32G32B32A32_SFLOAT, this);

                const VkDescriptorImageInfo inputTexture = { VK_NULL_HANDLE, envTextureEquirect.m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                const VkDescriptorImageInfo outputTexture = { VK_NULL_HANDLE, envTextureUnfiltered.m_texture_image_view, VK_IMAGE_LAYOUT_GENERAL };
                UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });
                UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

                VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer(m_command_buffers[0]);
                {
                    const auto preDispatchBarrier = ImageMemoryBarrier(envTextureUnfiltered, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL).mipLevels(0, 1);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, { preDispatchBarrier });

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
                    vkCmdDispatch(commandBuffer, kEnvMapSize / 32, kEnvMapSize / 32, 6);

                    const auto postDispatchBarrier = ImageMemoryBarrier(envTextureUnfiltered, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(0, 1);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { postDispatchBarrier });
                }
                // TODO: fix
                ExecuteImmediateCommandBuffer(commandBuffer, m_graphics_queue);
                //ExecuteImmediateCommandBuffer(commandBuffer, m_present_queue);

                vkDestroyPipeline(m_device, pipeline, nullptr);
                //destroyTexture(envTextureEquirect);

                GenerateMipmaps(envTextureUnfiltered);
            }

            VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer(m_command_buffers[0]);

            // Compute pre-filtered specular environment map.
            {
                const uint32_t numMipTailLevels = kEnvMapLevels - 1;

                VkPipeline pipeline;
                {
                    const VkSpecializationMapEntry specializationMap = { 0, 0, sizeof(uint32_t) };
                    const uint32_t specializationData[] = { numMipTailLevels };

                    const VkSpecializationInfo specializationInfo = { 1, &specializationMap, sizeof(specializationData), specializationData };
                    pipeline = CreateComputePipeline(m_device, "../shaders/spmap.spv", computePipelineLayout, &specializationInfo);
                }

                // Copy base mipmap level into destination environment map.
                {
                    const std::vector<ImageMemoryBarrier> preCopyBarriers = {
                        ImageMemoryBarrier(envTextureUnfiltered, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(0, 1),
                        ImageMemoryBarrier(*m_envTexture, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
                    };
                    const std::vector<ImageMemoryBarrier> postCopyBarriers = {
                        ImageMemoryBarrier(envTextureUnfiltered, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL).mipLevels(0, 1),
                        ImageMemoryBarrier(*m_envTexture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL),
                    };

                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, preCopyBarriers);

                    VkImageCopy copyRegion = {};
                    copyRegion.extent = { m_envTexture->m_width, m_envTexture->m_height, 1 };
                    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.srcSubresource.layerCount = m_envTexture->m_layers;
                    copyRegion.dstSubresource = copyRegion.srcSubresource;
                    vkCmdCopyImage(commandBuffer,
                        envTextureUnfiltered.m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        m_envTexture->m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &copyRegion);

                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, postCopyBarriers);
                }

                // Pre-filter rest of the mip-chain.
                std::vector<VkImageView> envTextureMipTailViews;
                {
                    std::vector<VkDescriptorImageInfo> envTextureMipTailDescriptors;
                    const VkDescriptorImageInfo inputTexture = { VK_NULL_HANDLE, envTextureUnfiltered.m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                    UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });

                    for (uint32_t level = 1; level < kEnvMapLevels; ++level) {
                        envTextureMipTailViews.push_back(CreateTextureView(m_device, *m_envTexture, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, level, 1));
                        envTextureMipTailDescriptors.push_back(VkDescriptorImageInfo{ VK_NULL_HANDLE, envTextureMipTailViews[level - 1], VK_IMAGE_LAYOUT_GENERAL });
                    }
                    UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_OutputMipTail, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, envTextureMipTailDescriptors);

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);

                    const float deltaRoughness = 1.0f / std::max(float(numMipTailLevels), 1.0f);
                    for (uint32_t level = 1, size = kEnvMapSize / 2; level < kEnvMapLevels; ++level, size /= 2) {
                        const uint32_t numGroups = std::max<uint32_t>(1, size / 32);

                        const SpecularFilterPushConstants pushConstants = { level - 1, level * deltaRoughness };
                        vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants), &pushConstants);
                        vkCmdDispatch(commandBuffer, numGroups, numGroups, 6);
                    }

                    const auto barrier = ImageMemoryBarrier(*m_envTexture, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { barrier });
                }

                // TODO: fix
                ExecuteImmediateCommandBuffer(commandBuffer, m_graphics_queue);

                for (VkImageView mipTailView : envTextureMipTailViews) {
                    vkDestroyImageView(m_device, mipTailView, nullptr);
                }
                vkDestroyPipeline(m_device, pipeline, nullptr);
                //destroyTexture(envTextureUnfiltered);
            }

            // Compute diffuse irradiance cubemap
            {
                VkPipeline pipeline = CreateComputePipeline(m_device, "shaders/spirv/irmap_cs.spv", computePipelineLayout);

                const VkDescriptorImageInfo inputTexture = { VK_NULL_HANDLE, m_envTexture->m_texture_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                const VkDescriptorImageInfo outputTexture = { VK_NULL_HANDLE, m_irmapTexture->m_texture_image_view, VK_IMAGE_LAYOUT_GENERAL };
                UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });
                UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

                VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer(m_command_buffers[0]);
                {
                    const auto preDispatchBarrier = ImageMemoryBarrier(*m_irmapTexture, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, { preDispatchBarrier });

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
                    vkCmdDispatch(commandBuffer, kIrradianceMapSize / 32, kIrradianceMapSize / 32, 6);

                    const auto postDispatchBarrier = ImageMemoryBarrier(*m_irmapTexture, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { postDispatchBarrier });
                }
                ExecuteImmediateCommandBuffer(commandBuffer, m_graphics_queue);
                vkDestroyPipeline(m_device, pipeline, nullptr);
            }

            // Compute Cook-Torrance BRDF 2D LUT for split-sum approximation.
            {
                VkPipeline pipeline = CreateComputePipeline(m_device, "shaders/spirv/spbrdf_cs.spv", computePipelineLayout);

                const VkDescriptorImageInfo outputTexture = { VK_NULL_HANDLE, m_spBRDF_LUT->m_texture_image_view, VK_IMAGE_LAYOUT_GENERAL };
                UpdateDescriptorSet(m_device, computeDescriptorSet, Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

                VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer(m_command_buffers[0]);
                {
                    const auto preDispatchBarrier = ImageMemoryBarrier(*m_spBRDF_LUT, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, { preDispatchBarrier });

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
                    vkCmdDispatch(commandBuffer, kBRDF_LUT_Size / 32, kBRDF_LUT_Size / 32, 6);

                    const auto postDispatchBarrier = ImageMemoryBarrier(*m_spBRDF_LUT, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { postDispatchBarrier });
                }
                ExecuteImmediateCommandBuffer(commandBuffer, m_graphics_queue);
                vkDestroyPipeline(m_device, pipeline, nullptr);
            }
        }

        // Clean up
        vkDestroyDescriptorSetLayout(m_device, setLayout.uniforms, nullptr);
        vkDestroyDescriptorSetLayout(m_device, setLayout.pbr, nullptr);
        vkDestroyDescriptorSetLayout(m_device, setLayout.skybox, nullptr);
        vkDestroyDescriptorSetLayout(m_device, setLayout.tonemap, nullptr);
        vkDestroyDescriptorSetLayout(m_device, setLayout.compute, nullptr);

        vkDestroySampler(m_device, m_compute_sampler, nullptr);
        vkDestroyPipelineLayout(m_device, computePipelineLayout, nullptr);
        vkDestroyDescriptorPool(m_device, computeDescriptorPool, nullptr);
    }

#if 0
    void GraphicsDevice::CreateGraphicsPipeline() {
        // Create Graphics Pipeline
        auto vert_shader_code = Utils::File::ReadFile("../shaders/basic/vert.spv");
        auto frag_shader_code = Utils::File::ReadFile("../shaders/basic/frag.spv");

        VkShaderModule vert_shader_module = vkUtilities::CreateShaderModule(vert_shader_code, m_device);
        VkShaderModule frag_shader_module = vkUtilities::CreateShaderModule(frag_shader_code, m_device);

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader_stage_info, frag_shader_stage_info };

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
        vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multi_sampling{};
        multi_sampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_sampling.sampleShadingEnable = VK_FALSE;
        multi_sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shaderStages;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &inputAssembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multi_sampling;
        pipeline_info.pDepthStencilState = &depthStencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = m_pipeline_layout;
        pipeline_info.renderPass = m_render_pass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_device, frag_shader_module, nullptr);
        vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
    }
#endif

    void GraphicsDevice::Draw(Camera* camera, Model* model) {
        vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device, m_swap_chain, UINT64_MAX, m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            LOG_ERROR(false, "Failed to acquire swap chain image!");
        }

        vkUtilities::UpdateUniformBuffers(camera, m_current_frame, m_swap_chain_extent, m_uniform_buffers_mapped);

        vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);

        vkResetCommandBuffer(m_command_buffers[m_current_frame], /*VkCommandBufferResetFlagBits*/ 0);
        vkUtilities::RecordCommandBuffer(model, m_descriptorsets.uniforms, m_command_buffers[m_current_frame], imageIndex, m_render_pass, m_swap_chain_extent, m_framebuffers,
            m_graphics_pipeline, nullptr, nullptr, 0, m_pipeline_layout, m_current_frame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_image_available_semaphores[m_current_frame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_command_buffers[m_current_frame];

        VkSemaphore signalSemaphores[] = { m_render_finished_semaphores[m_current_frame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_graphics_queue, 1, &submitInfo, m_in_flight_fences[m_current_frame]) != VK_SUCCESS) {
            LOG_ERROR(false, "failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swap_chain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(m_present_queue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized) {
            m_framebuffer_resized = false;
            CleanUpSwapchain();
        }
        else if (result != VK_SUCCESS) {
            LOG_ERROR(false, "failed to present swap chain image!");
        }
        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void GraphicsDevice::CreateSwapchain()
    {
        //		--Create Swap Chain--
        SwapChainSupportDetails swap_chain_support = vkUtilities::QuerySwapChainSupport(m_physical_device, m_surface);

        VkSurfaceFormatKHR surfaceFormat = vkUtilities::ChooseSwapSurfaceFormat(swap_chain_support.formats);
        VkPresentModeKHR presentMode = vkUtilities::ChooseSwapPresentMode(swap_chain_support.presentModes);
        VkExtent2D extent = vkUtilities::ChooseSwapExtent(swap_chain_support.capabilities, m_window->window());

        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swap_chain_create_info{};
        swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_chain_create_info.surface = m_surface;

        swap_chain_create_info.minImageCount = image_count;
        swap_chain_create_info.imageFormat = surfaceFormat.format;
        swap_chain_create_info.imageColorSpace = surfaceFormat.colorSpace;
        swap_chain_create_info.imageExtent = extent;
        swap_chain_create_info.imageArrayLayers = 1;
        swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swap_chain_create_info.queueFamilyIndexCount = 2;
            swap_chain_create_info.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swap_chain_create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swap_chain_create_info.presentMode = presentMode;
        swap_chain_create_info.clipped = VK_TRUE;

        swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &swap_chain_create_info, nullptr, &m_swap_chain) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
        m_swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

        m_swap_chain_image_format = surfaceFormat.format;
        m_swap_chain_extent = extent;
    }

    void GraphicsDevice::CleanUpSwapchain() {
        vkDestroyImageView(m_device, m_depth_image_view, nullptr);
        vkDestroyImage(m_device, m_depth_image, nullptr);
        vkFreeMemory(m_device, m_depth_image_memory, nullptr);
        for (auto framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        for (auto imageView : m_swap_chain_image_views)
            vkDestroyImageView(m_device, imageView, nullptr);

        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
    }

    void GraphicsDevice::CleanUp(const Config& config)
    {
        glfwWaitEvents();
        vkDeviceWaitIdle(m_device);

        CleanUpSwapchain();

        //vkDestroySampler(m_device, m_texture_sampler, nullptr);
        //vkDestroyImageView(m_device, m_texture_image_view, nullptr);

        //vkDestroyImage(m_device, m_texture_image, nullptr);
        //vkFreeMemory(m_device, m_texture_image_memory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_device, m_uniform_buffers[i], nullptr);
            vkFreeMemory(m_device, m_uniform_buffers_memory[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
        //vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
        //vkDestroyBuffer(m_device, m_index_buffer, nullptr);
        //vkFreeMemory(m_device, m_index_buffer_memory, nullptr);
        
        //vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
        //vkFreeMemory(m_device, m_vertex_buffer_memory, nullptr);
        vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
            vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_command_pool, nullptr);
        vkDestroyDevice(m_device, nullptr);
        if (config.enable_validation_layers)
            vkUtilities::DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        m_window->DestroyWindow();
        glfwTerminate();
    }

    void GraphicsDevice::RecreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window->window(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window->window(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device);

        CleanUpSwapchain();


        // create swap chain
        SwapChainSupportDetails swapchainsupport = vkUtilities::QuerySwapChainSupport(m_physical_device, m_surface);

        VkSurfaceFormatKHR surfaceFormat = vkUtilities::ChooseSwapSurfaceFormat(swapchainsupport.formats);
        VkPresentModeKHR presentmode = vkUtilities::ChooseSwapPresentMode(swapchainsupport.presentModes);
        VkExtent2D extent = vkUtilities::ChooseSwapExtent(swapchainsupport.capabilities, m_window->window());

        uint32_t imageCount = swapchainsupport.capabilities.minImageCount + 1;
        if (swapchainsupport.capabilities.maxImageCount> 0 && imageCount > swapchainsupport.capabilities.maxImageCount) {
            imageCount = swapchainsupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapchainsupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentmode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swap_chain) != VK_SUCCESS) {
            LOG_ERROR(false, "Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &imageCount, nullptr);
        m_swap_chain_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &imageCount, m_swap_chain_images.data());

        m_swap_chain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
        m_swap_chain_extent = extent;

        m_swap_chain_image_views.resize(m_swap_chain_images.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_swap_chain_images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swap_chain_image_views[i]) != VK_SUCCESS) {
                LOG_ERROR(false, "Failed to create image views!");
            }
        }

        m_framebuffers.resize(m_swap_chain_image_views.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
            VkImageView attachments[] = {
                m_swap_chain_image_views[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_render_pass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swap_chain_extent.width;
            framebufferInfo.height = m_swap_chain_extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
                LOG_ERROR(false, "Failed to create framebuffer!");
            }
        }
    }

    void GraphicsDevice::GenerateMipmaps(const Texture2D& texture)
    {
        //assert(texture.levels > 1);

        VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer(m_command_buffers[0]);

        // Iterate through mip chain and consecutively blit from previous level to next level with linear filtering.
        for (uint32_t level = 1, prevLevelWidth = texture.m_width, prevLevelHeight = texture.m_height; level < texture.m_mipLevels; ++level, prevLevelWidth /= 2, prevLevelHeight /= 2) {

            const auto preBlitBarrier = ImageMemoryBarrier(texture, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).mipLevels(level, 1);
            PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, { preBlitBarrier });

            VkImageBlit region = {};
            region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, texture.m_layers };
            region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level,   0, texture.m_layers };
            region.srcOffsets[1] = { int32_t(prevLevelWidth),  int32_t(prevLevelHeight),   1 };
            region.dstOffsets[1] = { int32_t(prevLevelWidth / 2),int32_t(prevLevelHeight / 2), 1 };
            vkCmdBlitImage(commandBuffer,
                texture.m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                texture.m_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region, VK_FILTER_LINEAR);

            const auto postBlitBarrier = ImageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(level, 1);
            PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, { postBlitBarrier });
        }

        // Transition whole mip chain to shader read only layout.
        {
            const auto barrier = ImageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { barrier });
        }

        ExecuteImmediateCommandBuffer(commandBuffer, m_graphics_queue);
    }
}