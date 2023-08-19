#pragma once

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
	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};
	class Graphics {
	public:
		bool Init(const Config& config);
		void Draw();
		void CleanUp(const Config& config);
		void CleanUpSwapchain();
		void RecreateSwapchain();
		void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }
		GLFWwindow* GetWindow() const { return m_window; }
	private:
		VkDevice m_device;
		GLFWwindow* m_window;
		VkInstance m_instance;
		VkSurfaceKHR m_surface;
		VkQueue m_present_queue;
		VkQueue m_graphics_queue;
		VkRenderPass m_render_pass;
		VkSwapchainKHR m_swap_chain;
		VkCommandPool m_command_pool;
		VkExtent2D m_swap_chain_extent;
		VkPipeline m_graphics_pipeline;
		VkPhysicalDevice m_physical_device;
		VkFormat m_swap_chain_image_format;
		VkPipelineLayout m_pipeline_layout;
		std::vector<VkFence> m_in_flight_fences;
		std::vector<VkImage> m_swap_chain_images;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		std::vector<VkCommandBuffer> m_command_buffers;
		std::vector<VkImageView> m_swap_chain_image_views;
		std::vector<VkFramebuffer> m_swap_chain_framebuffers; 
		std::vector<VkSemaphore> m_render_finished_semaphores;
		std::vector<VkSemaphore> m_image_available_semaphores;

		int m_width = 1200;
		int m_height = 720;

		int m_current_frame = 0;
		bool m_framebuffer_resized = false;
		const int MAX_FRAMES_IN_FLIGHT = 2;
	};
}