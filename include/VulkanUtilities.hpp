#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <optional>

#include "Model.hpp"

namespace Diffuse {

	struct Vertex;
	
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	class vkUtilities {
	public:
		static bool CheckValidationLayerSupport();
		static std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers);
		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		static bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions);
		static bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& device_extensions);
		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR m_surface);
		static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* m_window);
		static VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device);
		static void RecordCommandBuffer(Model* model, VkCommandBuffer command_buffer, uint32_t image_index, VkRenderPass render_pass, VkExtent2D swap_chain_extent,
			std::vector<VkFramebuffer> swap_chain_framebuffers, VkPipeline graphics_pipeline, VkBuffer vertex_buffer, VkBuffer index_buffer, int indices_size,
			VkPipelineLayout pipeline_layout, int current_frame);
		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device);
		static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, 
			VkPhysicalDevice physical_device, VkDevice device);
		static void CreateVertexBuffer(const std::vector<Vertex>& vertices, VkDevice device, VkBuffer vertex_buffer, VkDeviceMemory vertex_buffer_memory,
			VkCommandPool command_pool, VkQueue graphics_queue, VkPhysicalDevice physical_device);
		static 	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue);
		static void UpdateUniformBuffers(uint32_t current_image, VkExtent2D swap_chain_extent, std::vector<void*> uniform_buffers_mapped);
		static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device);
		static VkFormat FindDepthFormat(VkPhysicalDevice physical_device);
		static void CreateImage(uint32_t width, uint32_t height, VkDevice device, VkPhysicalDevice physical_device, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device);
		static void TransitionImageLayout(VkQueue graphics_queue, VkCommandPool command_pool, VkDevice device,
			VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		static void CopyBufferToImage(VkQueue graphics_queue, VkCommandPool command_pool, VkDevice device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		static VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool, VkDevice device);
		static void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphics_queue, VkCommandPool command_pool);
		static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format);
		static void DrawNode(Model* model, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Model::Node* node);
	};
}