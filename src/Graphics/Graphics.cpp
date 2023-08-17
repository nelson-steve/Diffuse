#include "Graphics.hpp"
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
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Diffuse Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Diffuse";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &appInfo;
		
		std::vector<const char*> extensions = vkUtilities::GetRequiredExtensions(config.enable_validation_layers);
		instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instance_create_info.ppEnabledExtensionNames = extensions.data();
		
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (config.enable_validation_layers) {
			instance_create_info.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
			instance_create_info.ppEnabledLayerNames = config.validation_layers.data();
		
			vkUtilities::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
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
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			std::cout << "failed to find GPUs with Vulkan support!";
			return false;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

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

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		device_create_info.pQueueCreateInfos = queueCreateInfos.data();

		device_create_info.pEnabledFeatures = &deviceFeatures;

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
		SwapChainSupportDetails swapChainSupport = vkUtilities::QuerySwapChainSupport(m_physical_device, m_surface);

		VkSurfaceFormatKHR surfaceFormat = vkUtilities::ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = vkUtilities::ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = vkUtilities::ChooseSwapExtent(swapChainSupport.capabilities, m_window);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swap_chain_create_info{};
		swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_create_info.surface = m_surface;

		swap_chain_create_info.minImageCount = imageCount;
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

		swap_chain_create_info.preTransform = swapChainSupport.capabilities.currentTransform;
		swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_chain_create_info.presentMode = presentMode;
		swap_chain_create_info.clipped = VK_TRUE;

		swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_device, &swap_chain_create_info, nullptr, &m_swap_chain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(m_device, m_swap_chain, &imageCount, nullptr);
		m_swap_chain_images.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swap_chain, &imageCount, m_swap_chain_images.data());

		m_swap_chain_image_format = surfaceFormat.format;
		m_swap_chain_extent = extent;

		// Create Image Views
		m_swap_chain_image_views.resize(m_swap_chain_images.size());

		for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swap_chain_images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swap_chain_image_format;
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
				std::cout<<"failed to create image views!";
				return false;
			}
		}

		// SUCCESS
		return true;
	}
}