#include "VulkanUtilities.hpp"
#include <vector>
namespace Diffuse {
	struct Config {
		bool enable_validation_layers = false;
		const std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};
	};
	class Graphics {
	public:
		bool Init(const Config& config);
		void Draw();
	private:
		GLFWwindow* m_window;
		VkInstance m_instance;
	};
}