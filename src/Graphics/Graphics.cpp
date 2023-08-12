#include "Graphics.hpp"
#include <iostream>

namespace Diffuse {
	bool Graphics::Init(const Config& config) {
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
			std::cout<<"Failed to create instance!";
			return false;
		}

		return true;
	}
}