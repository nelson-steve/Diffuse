#pragma once

#include "VulkanUtilities.hpp"
#include "Window.hpp"

#include <glm/glm.hpp>

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
    class GraphicsDevice {
    private:
        // == WINDOW ======================================
        std::shared_ptr<Window>                     m_window;
        // == VULKAN HANDLES =================================== 
        VkDevice                        m_device;
        VkInstance                      m_instance;
        VkQueue                         m_present_queue;
        VkBuffer                        m_index_buffer;
        VkBuffer                        m_vertex_buffer;
        VkQueue                         m_graphics_queue;
        VkExtent2D                      m_swap_chain_extent;
        VkPipeline                      m_graphics_pipeline;
        VkFormat                        m_swap_chain_image_format;
        VkSurfaceKHR                    m_surface;
        VkRenderPass                    m_render_pass;
        VkCommandPool                   m_command_pool;
        VkSwapchainKHR                  m_swap_chain;
        VkPhysicalDevice                m_physical_device;
        VkPipelineLayout                m_pipeline_layout;
        VkDeviceMemory                  m_index_buffer_memory;
        VkDeviceMemory                  m_vertex_buffer_memory;
        std::vector<VkFence>            m_in_flight_fences;
        std::vector<VkImage>            m_swap_chain_images;
        VkDebugUtilsMessengerEXT        m_debug_messenger;
        std::vector<VkImageView>        m_swap_chain_image_views;
        std::vector<VkSemaphore>        m_render_finished_semaphores;
        std::vector<VkSemaphore>        m_image_available_semaphores;
        std::vector<VkFramebuffer>      m_swap_chain_framebuffers;
        std::vector<VkCommandBuffer>    m_command_buffers;
        // =====================================================
    public:
        GraphicsDevice();
        ~GraphicsDevice();
        //bool Init(const Config& config);
        void Draw();
        void CleanUp(const Config& config);
        void CleanUpSwapchain();
        void RecreateSwapchain();
        void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }

        std::shared_ptr<Window> GetWindow() const { return m_window; }

        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };

        const std::vector<uint16_t> m_indices = {
            0, 1, 2, 2, 3, 0
        };

        int m_current_frame = 0;
        bool m_framebuffer_resized = false;
        const int MAX_FRAMES_IN_FLIGHT = 2;
    };
}