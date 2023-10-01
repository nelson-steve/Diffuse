#pragma once

#include "VulkanUtilities.hpp"
#include "Window.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

namespace Diffuse {

    class Mesh;

    struct Config {
        bool enable_validation_layers = false;
        const std::vector<const char*> validation_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> required_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex_coords;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, tex_coords);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const {
            return 
                pos         == other.pos        &&
                color       == other.color      &&
                tex_coords  == other.tex_coords &&
                normal      == other.normal;
        }
    };

    class GraphicsDevice {
    private:
        // == WINDOW HANDLE ====================================
        // Window handle using GLFW 
        std::shared_ptr<Window>         m_window;
        // == VULKAN HANDLES ===================================
        VkDevice                        m_device;
        VkFormat                        m_swap_chain_image_format;
        VkQueue                         m_present_queue;
        //VkBuffer                        m_index_buffer;
        //VkBuffer                        m_vertex_buffer;
        //VkImage                         m_texture_image;
        VkQueue                         m_graphics_queue;
        VkImage                         m_depth_image;
        //VkSampler                       m_texture_sampler;
        VkInstance                      m_instance;
        VkExtent2D                      m_swap_chain_extent;
        VkPipeline                      m_graphics_pipeline;
        VkImageView                     m_depth_image_view;
        //VkImageView                     m_texture_image_view;
        VkSurfaceKHR                    m_surface;
        VkRenderPass                    m_render_pass;
        VkCommandPool                   m_command_pool;
        //VkDeviceMemory                  m_texture_image_memory;
        VkDeviceMemory                  m_index_buffer_memory;
        VkDeviceMemory                  m_depth_image_memory;
        VkSwapchainKHR                  m_swap_chain;
        VkDeviceMemory                  m_vertex_buffer_memory;
        VkDescriptorPool                m_descriptor_pool;
        VkPipelineLayout                m_pipeline_layout;
        VkPhysicalDevice                m_physical_device;
        std::vector<void*>              m_uniform_buffers_mapped;
        std::vector<VkFence>            m_in_flight_fences;
        std::vector<VkImage>            m_swap_chain_images;
        std::vector<VkBuffer>           m_uniform_buffers;
        VkDescriptorSetLayout           m_descriptor_set_layout;
        VkDescriptorSetLayout           m_textures_descriptor_set_layout;
        VkDebugUtilsMessengerEXT        m_debug_messenger;
        std::vector<VkImageView>        m_swap_chain_image_views;
        std::vector<VkSemaphore>        m_render_finished_semaphores;
        std::vector<VkSemaphore>        m_image_available_semaphores;
        std::vector<VkFramebuffer>      m_swap_chain_framebuffers;
        std::vector<VkDeviceMemory>     m_uniform_buffers_memory;
        VkDescriptorSet                 m_descriptor_set_ubo;
        std::vector<VkDescriptorSet>    m_descriptor_sets_textures;
        std::vector<VkCommandBuffer>    m_command_buffers;
        // =====================================================
    public:
        // @brief - Constructor: Initializes Vulkan and creates a Vulkan Device and creates a window.
        GraphicsDevice(Config config = {});

        // TODO: Think of a better place to put Draw function. Hint: Renderer class
        void Draw(Model* model);

        void LoadModel();
        void CreateTexture(const std::string& path);
        void CreateVertexBuffer(VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory, const std::vector<Vertex> vertices);
        void CreateIndexBuffer(VkBuffer& index_buffer, VkDeviceMemory& index_buffer_memory, const std::vector<uint32_t> vertices);
        void CreateDescriptorSet(Model& model);
        void CreateGraphicsPipeline();
        void CreateSwapchain();
        void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }
        void RecreateSwapchain();
        void CleanUp(const Config& config = {});
        void CleanUpSwapchain();

        std::shared_ptr<Window> GetWindow() const { return m_window; }
        //int m_indices_size;
        int m_current_frame = 0;
        bool m_framebuffer_resized = false;
        const int MAX_FRAMES_IN_FLIGHT = 1;

        friend class Model;
        friend class Texture2D;
    };
}

namespace std {
    template<> struct hash<Diffuse::Vertex> {
        size_t operator()(Diffuse::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_coords) << 1);
        }
    };
}