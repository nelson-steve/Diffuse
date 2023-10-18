#pragma once

#include "VulkanUtilities.hpp"
#include "Window.hpp"
#include "Buffer.hpp"
#include "Swapchain.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

namespace Diffuse {
    struct Config {
        bool enable_validation_layers = true;
        const std::vector<const char*> validation_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> required_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };

    struct ObjectMaterial {
        // Parameter block used as push constant block
        struct PushBlock {
            float roughness = 0.0f;
            float metallic = 0.0f;
            float specular = 0.0f;
            float r, g, b;
        } params;
        std::string name;
        ObjectMaterial() {};
        ObjectMaterial(std::string n, glm::vec3 c) : name(n) {
            params.r = c.r;
            params.g = c.g;
            params.b = c.b;
        };
    };

    struct UBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    class GraphicsDevice {
    public:
        // Constructor: Initializes Vulkan and creates a Vulkan Device and creates a window.
        GraphicsDevice(Config config = {});
        void Setup();

        // Getters
        std::shared_ptr<Window> GetWindow() const { return m_window; }
        const VkDevice& Device() const { return m_device; }
        const VkQueue& Queue() const { return m_graphics_queue; }
        const VkCommandPool& CommandPool() const { return m_command_pool; }
        const VkPhysicalDevice& PhysicalDevice() const { return m_physical_device; }
        const VkSurfaceKHR& Surface() const { return m_surface; }

        void Draw(Camera* camera);
        void DrawNode(Node* node, VkCommandBuffer commandBuffer);

        void CreateVertexBuffer(VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory, uint32_t buffer_size, const Vertex* vertices);
        void CreateIndexBuffer(VkBuffer& index_buffer, VkDeviceMemory& index_buffer_memory, uint32_t buffer_size, const uint32_t* indices);
        void CreateUniformBuffer();

        void RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
        void CreateGraphicsPipeline();

        // Swapchain
        void CreateSwapchain();
        void RecreateSwapchain();

        void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }
        //static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

        // Cleanup
        void CleanUp(const Config& config = {});
        void CleanUpSwapchain();

        //friend class Model;
        //friend class Texture2D;
        std::vector<Model*> m_models;

    private:
        std::shared_ptr<Window>         m_window;
        // == VULKAN HANDLES ===================================
        VkQueue                         m_present_queue;
        VkQueue                         m_graphics_queue;
        VkImage                         m_depth_image;
        VkRect2D                        m_frame_rect;
        VkDevice                        m_device;
        //VkFormat                        m_swap_chain_image_format;
        VkSampler                       m_compute_sampler;
        VkSampler                       m_default_sampler;
        VkSampler                       m_brdf_sampler;
        VkInstance                      m_instance;
        //VkExtent2D                      m_swap_chain_extent;
        VkImageView                     m_depth_image_view;
        VkSubmitInfo                    m_submit_info;
        VkSurfaceKHR                    m_surface;
        VkRenderPass                    m_render_pass;
        VkCommandPool                   m_command_pool;
        VkDeviceMemory                  m_index_buffer_memory;
        VkDeviceMemory                  m_depth_image_memory;
        std::unique_ptr<Swapchain>      m_swapchain;
        //VkSwapchainKHR                  m_swap_chain;
        VkDeviceMemory                  m_vertex_buffer_memory;
        VkPipelineCache                 m_pipeline_cache;
        VkDescriptorPool                m_descriptor_pool;
        VkPipelineLayout                m_pipeline_layout;
        VkPhysicalDevice                m_physical_device;
        std::vector<void*>              m_uniform_buffers_mapped;
        std::vector<VkFence>            imagesInFlightFences;
        //std::vector<VkImage>            m_swap_chain_images;
        std::vector<VkBuffer>           m_uniform_buffers;
        VkDebugUtilsMessengerEXT        m_debug_messenger;
        //std::vector<VkImageView>        m_swap_chain_image_views;
        std::vector<VkFramebuffer>      m_framebuffers;
        std::vector<VkDeviceMemory>     m_uniform_buffers_memory;
        std::vector<VkCommandBuffer>    m_command_buffers;
        VkDebugUtilsMessengerCreateInfoEXT m_debug_create_info;
        //std::unordered_map<std::string, VkPipeline> pipelines;
        //VkPipeline boundPipeline;

        struct DescriptorSetLayouts {
            VkDescriptorSetLayout empty;
            VkDescriptorSetLayout scene;
            VkDescriptorSetLayout material;
            VkDescriptorSetLayout node;
            VkDescriptorSetLayout materialBuffer;
        } m_descriptorSetLayouts;

        struct Pipelines {
            VkPipeline default_;
        } m_pipelines;

        struct DescriptorSets {
            std::vector<VkDescriptorSet> scene;
            VkDescriptorSet skybox;
        } m_descriptor_sets;

        struct {
            std::vector<VkBuffer> uniformBuffers;
            std::vector<VkDeviceMemory> uniformBuffersMemory;
            std::vector<void*> uniformBuffersMapped;
        } m_ubo;

        std::vector<VkFence> m_wait_fences;
        //std::vector<DescriptorSets> m_descriptor_sets;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> m_render_complete_semaphores;
        std::vector<VkSemaphore> m_present_complete_semaphores;

        // Other variables
        uint32_t m_current_frame_index = 0;
        uint32_t m_render_ahead = 2;
        bool m_framebuffer_resized = false;
        uint32_t m_render_samples = 0;
        // =====================================================
    };
}