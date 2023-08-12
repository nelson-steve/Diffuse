#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace Diffuse {
	class vkUtilities {
	public:
		static bool CheckValidationLayerSupport();
		static std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers);
		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	};
}