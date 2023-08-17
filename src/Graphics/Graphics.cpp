#include "Graphics.hpp"
#include "ReadFile.hpp"

#include <iostream>
#include <set>

namespace Diffuse {
	bool Graphics::Init(const Config& config) {
		if (glfwInit() != GLFW_TRUE)
		{
			std::cout << "Failed to intitialize GLFW";
			return false;
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(m_width, m_height, "Diffuse", nullptr, nullptr);

		//		---Create Vulkan Instancee---
		if (config.enable_validation_layers && !vkUtilities::CheckValidationLayerSupport()) {
			std::cout << "validation layers requested, but not available!";
			return false;
		}
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Diffuse Vulkan Renderer";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Diffuse";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &app_info;
		
		std::vector<const char*> extensions = vkUtilities::GetRequiredExtensions(config.enable_validation_layers);
		instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instance_create_info.ppEnabledExtensionNames = extensions.data();
		
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
		if (config.enable_validation_layers) {
			instance_create_info.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
			instance_create_info.ppEnabledLayerNames = config.validation_layers.data();
		
			vkUtilities::PopulateDebugMessengerCreateInfo(debug_create_info);
			instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
		}
		else {
			instance_create_info.enabledLayerCount = 0;
		
			instance_create_info.pNext = nullptr;
		}
		
		if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS) {
			std::cout<<"Failed to create Vulkan instance!";
			return false;
		}
		
		//		--Setup Debug Messenger--
		if (config.enable_validation_layers) {
			VkDebugUtilsMessengerCreateInfoEXT messenger_create_info;
			vkUtilities::PopulateDebugMessengerCreateInfo(messenger_create_info);

			if (vkUtilities::CreateDebugUtilsMessengerEXT(m_instance, &messenger_create_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
				std::cout<<"Failed to set up debug messenger";
				return false;
			}
		}
		//		--Create Surface--
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			std::cout<<"failed to create window surface!";
			return false;
		}

		//		--Pick Physical Device--
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

		if (device_count == 0) {
			std::cout << "failed to find GPUs with Vulkan support!";
			return false;
		}

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

		for (const auto& device : devices) {
			if (vkUtilities::IsDeviceSuitable(device, m_surface,config.device_extensions)) {
				m_physical_device = device;
				break;
			}
		}

		if (m_physical_device == VK_NULL_HANDLE) {
			std::cout << "failed to find a suitable GPU!";
			return false;
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

		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		device_create_info.pQueueCreateInfos = queue_create_infos.data();

		device_create_info.pEnabledFeatures = &device_features;

		device_create_info.enabledExtensionCount = 0;

		device_create_info.enabledExtensionCount = static_cast<uint32_t>(config.device_extensions.size());
		device_create_info.ppEnabledExtensionNames = config.device_extensions.data();

		if (config.enable_validation_layers) {
			device_create_info.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
			device_create_info.ppEnabledLayerNames = config.validation_layers.data();
		}
		else {
			device_create_info.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS) {
			std::cout << "failed to create logical device!";
			return false;
		}

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphics_queue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_present_queue);

		//		--Create Swap Chain--
		SwapChainSupportDetails swap_chain_support = vkUtilities::QuerySwapChainSupport(m_physical_device, m_surface);

		VkSurfaceFormatKHR surfaceFormat = vkUtilities::ChooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR presentMode = vkUtilities::ChooseSwapPresentMode(swap_chain_support.presentModes);
		VkExtent2D extent = vkUtilities::ChooseSwapExtent(swap_chain_support.capabilities, m_window);

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

		//QueueFamilyIndices indices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);
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
			std::cout << "failed to create swap chain!";
			return false;
		}

		vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
		m_swap_chain_images.resize(image_count);
		vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

		m_swap_chain_image_format = surfaceFormat.format;
		m_swap_chain_extent = extent;

		// Create Image Views
		m_swap_chain_image_views.resize(m_swap_chain_images.size());

		for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
			VkImageViewCreateInfo image_views_create_info{};
			image_views_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_views_create_info.image = m_swap_chain_images[i];
			image_views_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_views_create_info.format = m_swap_chain_image_format;
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
				std::cout << "failed to create image views!";
				return false;
			}
		}

		// Create Render Pass
		VkAttachmentDescription color_attachment_desc{};
		color_attachment_desc.format = m_swap_chain_image_format;
		color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment_desc;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS) {
			std::cout << "failed to create render pass!";
			return false;
		}

		// Create Graphics Pipeline
		auto vert_shader_code = Utils::File::ReadFile("../shaders/vert.spv");
		auto frag_shader_code = Utils::File::ReadFile("../shaders/frag.spv");

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
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.vertexAttributeDescriptionCount = 0;

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
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multi_sampling{};
		multi_sampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multi_sampling.sampleShadingEnable = VK_FALSE;
		multi_sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
			std::cout << "failed to create pipeline layout!";
			return false;
		}

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shaderStages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &inputAssembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multi_sampling;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = m_pipeline_layout;
		pipeline_info.renderPass = m_render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS) {
			std::cout << "failed to create graphics pipeline!";
			return false;
		}

		vkDestroyShaderModule(m_device, frag_shader_module, nullptr);
		vkDestroyShaderModule(m_device, vert_shader_module, nullptr);

		// Create Framebuffers
		m_swap_chain_framebuffers.resize(m_swap_chain_image_views.size());

		for (size_t i = 0; i < m_swap_chain_image_views.size(); i++) {
			VkImageView attachments[] = {
				m_swap_chain_image_views[i]
			};

			VkFramebufferCreateInfo framebuffer_info{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = m_render_pass;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = m_swap_chain_extent.width;
			framebuffer_info.height = m_swap_chain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swap_chain_framebuffers[i]) != VK_SUCCESS) {
				std::cout << "failed to create framebuffer!";
				return false;
			}
		}

		// Create Command Pool
		//QueueFamilyIndices queueFamilyIndices = vkUtilities::FindQueueFamilies(m_physical_device, m_surface);

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = indices.graphicsFamily.value();

		if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS) {
			std::cout << "failed to create command pool!";
			return false;
		}

		// Create Command Buffer
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = m_command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_device, &alloc_info, &m_command_buffer) != VK_SUCCESS) {
			std::cout << "failed to allocate command buffers!";
			return false;
		}

		// Create Sync Obects
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphore) != VK_SUCCESS ||
			vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fence) != VK_SUCCESS) {
			std::cout << "failed to create synchronization objects for a frame!";
			return false;
		}

		// SUCCESS
		return true;
	}
	void Graphics::Draw()
	{
		vkWaitForFences(m_device, 1, &m_in_flight_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_in_flight_fence);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_device, m_swap_chain, UINT64_MAX, m_image_available_semaphore, VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(m_command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
		vkUtilities::RecordCommandBuffer(m_command_buffer, imageIndex, m_render_pass, m_swap_chain_extent, m_swap_chain_framebuffers, m_graphics_pipeline);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_image_available_semaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_command_buffer;

		VkSemaphore signalSemaphores[] = { m_render_finished_semaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_graphics_queue, 1, &submitInfo, m_in_flight_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_swap_chain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(m_present_queue, &presentInfo);
	}
}