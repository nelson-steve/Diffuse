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
        // Constructor: Initializes Vulkan instances and creates a window
        GraphicsDevice(Config config = {});
        void Setup();
        void SetupSkybox();
        void SetupOffscreenResources();

        // Getters
        std::shared_ptr<Window> GetWindow() const { return m_window; }
        const VkDevice& Device() const { return m_device; }
        const VkQueue& Queue() const { return m_graphics_queue; }
        const VkCommandPool& CommandPool() const { return m_command_pool; }
        const VkPhysicalDevice& PhysicalDevice() const { return m_physical_device; }
        const VkSurfaceKHR& Surface() const { return m_surface; }

        void Draw(Camera* camera);
        void DrawNode(Node* node, VkCommandBuffer commandBuffer);
        void DrawNodeSkybox(Node* node, VkCommandBuffer commandBuffer);

        void CreateVertexBuffer(VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory, uint32_t buffer_size, const Vertex* vertices);
        void CreateIndexBuffer(VkBuffer& index_buffer, VkDeviceMemory& index_buffer_memory, uint32_t buffer_size, const uint32_t* indices);
        void CreateUniformBuffer();

        void RecordCommandBuffer(Camera* camera, VkCommandBuffer command_buffer, uint32_t image_index);
        void CreateGraphicsPipeline();

        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false)
        {
            VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
            cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufAllocateInfo.commandPool = m_command_pool;
            cmdBufAllocateInfo.level = level;
            cmdBufAllocateInfo.commandBufferCount = 1;

            VkCommandBuffer cmdBuffer;
            if (vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &cmdBuffer) != VK_SUCCESS) {
                assert(false);
            }

            // If requested, also start recording for the new command buffer
            if (begin) {
                VkCommandBufferBeginInfo commandBufferBI{};
                commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                if (vkBeginCommandBuffer(cmdBuffer, &commandBufferBI) != VK_SUCCESS) {
                    assert(false);
                }
            }

            return cmdBuffer;
        }

        void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true)
        {
            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                assert(false);
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            // Create fence to ensure that the command buffer has finished executing
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkFence fence;
            if (vkCreateFence(m_device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                assert(false);
            }

            // Submit to the queue
            if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
                assert(false);
            }
            // Wait for the fence to signal that command buffer has finished executing
            if (vkWaitForFences(m_device, 1, &fence, VK_TRUE, 100000000000) != VK_SUCCESS) {
                assert(false);
            }

            vkDestroyFence(m_device, fence, nullptr);

            if (free) {
                vkFreeCommandBuffers(m_device, m_command_pool, 1, &commandBuffer);
            }
        }

        // Swapchain
        void RecreateSwapchain();

        void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }
        //static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

        // Cleanup
        void CleanUp(const Config& config = {});
        void CleanUpSwapchain();

        //friend class Model;
        //friend class Texture2D;
        std::vector<Model*> m_models;
        Texture2D* white_texture;

    private:
        std::shared_ptr<Window>         m_window;
        // == VULKAN HANDLES ===================================
        VkQueue                         m_present_queue;
        VkQueue                         m_graphics_queue;
        VkImage                         m_depth_image;
        VkRect2D                        m_frame_rect;
        VkDevice                        m_device;
        VkSampler                       m_compute_sampler;
        VkSampler                       m_skybox_sampler;
        VkSampler                       m_brdf_sampler;
        VkInstance                      m_instance;
        VkImageView                     m_depth_image_view;
        VkSubmitInfo                    m_submit_info;
        VkSurfaceKHR                    m_surface;
        VkRenderPass                    m_render_pass;
        VkRenderPass                    m_offscreen_render_pass;
        VkCommandPool                   m_command_pool;
        VkDeviceMemory                  m_index_buffer_memory;
        VkDeviceMemory                  m_depth_image_memory;
        std::unique_ptr<Swapchain>      m_swapchain;
        VkDeviceMemory                  m_vertex_buffer_memory;
        VkPipelineCache                 m_pipeline_cache;
        //VkPipelineLayout                m_pipeline_layout;
        VkPhysicalDevice                m_physical_device;
        std::vector<void*>              m_uniform_buffers_mapped;
        std::vector<VkFence>            imagesInFlightFences;
        std::vector<VkBuffer>           m_uniform_buffers;
        VkDebugUtilsMessengerEXT        m_debug_messenger;
        std::vector<VkFramebuffer>      m_framebuffers;
        std::array<VkFramebuffer, 6>    m_offscreen_framebuffers;
        std::vector<VkDeviceMemory>     m_uniform_buffers_memory;
        std::vector<VkCommandBuffer>    m_command_buffers;
        VkDebugUtilsMessengerCreateInfoEXT m_debug_create_info;
        //std::unordered_map<std::string, VkPipeline> pipelines;
        //VkPipeline boundPipeline;

        struct SpecularFilterPushConstants
        {
            uint32_t level;
            float roughness;
        };

        struct DescriptorPools {
            VkDescriptorPool scene;
            VkDescriptorPool compute;
        } m_descriptor_pools;

        struct DescriptorSetLayouts {
            VkDescriptorSetLayout empty;
            VkDescriptorSetLayout scene;
            VkDescriptorSetLayout skybox;
            VkDescriptorSetLayout compute;
            VkDescriptorSetLayout material;
            VkDescriptorSetLayout node;
            VkDescriptorSetLayout materialBuffer;
        } m_descriptorSetLayouts;

        struct PipelineLayouts{
            VkPipelineLayout scene;
            VkPipelineLayout skybox;
            VkPipelineLayout compute;
        } m_pipeline_layouts;
        struct Pipelines {
            VkPipeline scene;
            VkPipeline skybox;
            VkPipeline compute;
            VkPipeline skybox_equirectangular;
        } m_pipelines;

        struct DescriptorSets {
            std::vector<VkDescriptorSet> scene;
            VkDescriptorSet skybox;
            VkDescriptorSet skybox_equirectangular;
            VkDescriptorSet compute;
        } m_descriptor_sets;

        struct {
            std::vector<VkBuffer> uniformBuffers;
            std::vector<VkDeviceMemory> uniformBuffersMemory;
            std::vector<void*> uniformBuffersMapped;
        } m_ubo;

        struct {
            VkImageView view;
            VkImage image;
            VkDeviceMemory memory;
            VkSampler sampler;
            VkImageLayout layout;
        } m_cubemap;

        struct {
            VkImageView view;
            VkImage image;
            VkDeviceMemory memory;
        } m_offscreen;
        std::array<VkImageView, 6> m_cubemap_face_image_views;
        VkSampler computeSampler;

        std::vector<VkFence> m_wait_fences;
        //std::vector<DescriptorSets> m_descriptor_sets;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> m_render_complete_semaphores;
        std::vector<VkSemaphore> m_present_complete_semaphores;

        // Other variables
        uint32_t m_current_frame_index = 0;
        uint32_t m_render_ahead = 1;
        bool m_framebuffer_resized = false;
        uint32_t m_render_samples = 0;
        Texture2D* hdr;
        bool only_once = false;
        uint32_t offscreen_size = 2048;
        // =====================================================
    };
}