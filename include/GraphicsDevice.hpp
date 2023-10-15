#pragma once

#include "VulkanUtilities.hpp"
#include "Window.hpp"
#include "Buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

namespace Diffuse {
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
#if 0
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
            attributeDescriptions[1].offset = offsetof(Vertex, normal);

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
                normal      == other.normal     &&
                tex_coords  == other.tex_coords &&
                color       == other.color;
        }
    };
#endif

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

    class GraphicsDevice {
    private:
        // == WINDOW HANDLE ====================================
        // Window handle using GLFW 
        std::shared_ptr<Window>         m_window;
        // == VULKAN HANDLES ===================================
        VkQueue                         m_present_queue;
        VkQueue                         m_graphics_queue;
        VkImage                         m_depth_image;
        VkRect2D                        m_frame_rect;
        VkDevice                        m_device;
        VkFormat                        m_swap_chain_image_format;
        VkSampler                       m_compute_sampler;
        VkSampler                       m_default_sampler;
        VkSampler                       m_brdf_sampler;
        VkInstance                      m_instance;
        VkExtent2D                      m_swap_chain_extent;
        //VkPipeline                      m_graphics_pipeline;
        VkImageView                     m_depth_image_view;
        VkSubmitInfo                    m_submit_info;
        VkSurfaceKHR                    m_surface;
        VkRenderPass                    m_render_pass;
        VkCommandPool                   m_command_pool;
        VkDeviceMemory                  m_index_buffer_memory;
        VkDeviceMemory                  m_depth_image_memory;
        VkSwapchainKHR                  m_swap_chain;
        VkDeviceMemory                  m_vertex_buffer_memory;
        VkPipelineCache                 m_pipeline_cache;
        VkDescriptorPool                m_descriptor_pool;
        VkPipelineLayout                m_pipeline_layout;
        VkPhysicalDevice                m_physical_device;
        std::vector<void*>              m_uniform_buffers_mapped;
        std::vector<VkFence>            m_in_flight_fences;
        std::vector<VkFence>            imagesInFlightFences;
        std::vector<VkImage>            m_swap_chain_images;
        std::vector<VkBuffer>           m_uniform_buffers;
        VkDebugUtilsMessengerEXT        m_debug_messenger;
        std::vector<VkImageView>        m_swap_chain_image_views;
        std::vector<VkSemaphore>        m_render_finished_semaphores;
        std::vector<VkSemaphore>        m_image_available_semaphores;
        //std::vector<VkFramebuffer>      m_swap_chain_framebuffers;
        std::vector<VkFramebuffer>      m_framebuffers;
        std::vector<VkDeviceMemory>     m_uniform_buffers_memory;
        //std::vector<VkDescriptorSet>    m_uniformsDescriptorSets;
        std::vector<VkCommandBuffer>    m_command_buffers;
        // =====================================================
    public:
        // @brief - Constructor: Initializes Vulkan and creates a Vulkan Device and creates a window.
        GraphicsDevice(Config config = {});

        VkDevice Device() const { return m_device; }
        VkPhysicalDevice PhysicalDevice() const { return m_physical_device; }
        VkCommandPool CommandPool() const { return m_command_pool; }
        VkQueue Queue() const { return m_graphics_queue; }

        void Setup(Camera* camera);
        void Draw(Camera* camera);

        void CreateVertexBuffer(VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory, const std::vector<Vertex> vertices);
        void CreateIndexBuffer(VkBuffer& index_buffer, VkDeviceMemory& index_buffer_memory, const std::vector<uint32_t> vertices);
        VkPipeline CreateGraphicsPipeline(uint32_t subpass,
            const std::string& vs, const std::string& fs, VkPipelineLayout layout,
            const std::vector<VkVertexInputBindingDescription>* vertexInputBindings = nullptr,
            const std::vector<VkVertexInputAttributeDescription>* vertexAttributes = nullptr,
            const VkPipelineMultisampleStateCreateInfo* multisampleState = nullptr,
            const VkPipelineDepthStencilStateCreateInfo* depthStencilState = nullptr);
        void CreateSwapchain();
        void SetFramebufferResized(bool resized) { m_framebuffer_resized = resized; }
        void RecreateSwapchain();
        void BuildCommandBuffers(uint32_t image_index);
        void CleanUp(const Config& config = {});
        void CleanUpSwapchain();

        void GenerateBRDFLUT();
        void GenerateIrradianceCube();
        void GeneratePrefilteredCube();

        struct Textures {
            TextureCubemap* environmentCube;
            // Generated at runtime
            Texture2D* lutBrdf;
            TextureCubemap* irradianceCube;
            TextureCubemap* prefilteredCube;
        } textures;

        struct Meshes {
            Model skybox;
            std::vector<Model> objects;
            int32_t objectIndex = 0;
        } models;

        struct {
            Buffer object;
            Buffer skybox;
            Buffer params;
        } uniformBuffers;

        struct UBOMatrices {
            glm::mat4 projection;
            glm::mat4 model;
            glm::mat4 view;
            glm::vec3 camPos;
        } uboMatrices;

        struct UBOParams {
            glm::vec4 lights[4];
            float exposure = 4.5f;
            float gamma = 2.2f;
        } uboParams;

        struct {
            VkPipeline skybox;
            VkPipeline pbr;
        } pipelines;

        struct {
            VkDescriptorSet object;
            VkDescriptorSet skybox;
        } descriptorSets;

        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;

        // Default materials to select from
        std::vector<ObjectMaterial> materials;
        int32_t materialIndex = 0;

        std::vector<std::string> materialNames;
        std::vector<std::string> objectNames;

        std::shared_ptr<Window> GetWindow() const { return m_window; }
        //int m_indices_size;
        int m_current_frame = 0;
        bool m_framebuffer_resized = false;
        const int MAX_FRAMES_IN_FLIGHT = 2;
        //const int m_numFrames = 1;
        uint32_t m_renderSamples;

        friend class Model;
        friend class Texture2D;
    };
}