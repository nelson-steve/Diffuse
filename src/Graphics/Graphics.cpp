#include "Graphics.hpp"
#include <iostream>

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

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		std::vector<const char*> extensions = vkUtilities::GetRequiredExtensions(config.enable_validation_layers);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (config.enable_validation_layers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(config.validation_layers.size());
			createInfo.ppEnabledLayerNames = config.validation_layers.data();
		
			vkUtilities::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
		
			createInfo.pNext = nullptr;
		}
		
		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
			std::cout<<"Failed to create Vulkan instance!";
			return false;
		}
		
		//		--Setup Debug Messenger--
		if (config.enable_validation_layers) {
			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			vkUtilities::PopulateDebugMessengerCreateInfo(createInfo);

			if (vkUtilities::CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debug_messenger) != VK_SUCCESS) {
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

		// SUCCESS
		return true;
	}
}