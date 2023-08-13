#include "VulkanUtilities.hpp"
#include <vector>
namespace Diffuse {
	struct Config {
		bool enable_validation_layers = false;
		const std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
	};
	class Graphics {
	public:
		bool Init(const Config& config);
		void Draw();
	private:
		GLFWwindow* m_window;
		VkInstance m_instance;
		VkSurfaceKHR m_surface;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;

		int m_width = 1200;
		int m_height = 720;
	};
}