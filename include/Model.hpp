#pragma once

#include "Texture2D.hpp"

#include "tiny_gltf.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan.h"
#include <iostream>

namespace Diffuse {

	class GraphicsDevice;

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv0;
		glm::vec2 uv1;
		glm::vec4 color;
	};

	struct Texture {
		GraphicsDevice* device;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;
		void updateDescriptor();
		void destroy();
		// Load a texture from a glTF image (stored as vector of chars loaded via stb_image) and generate a full mip chaing for it
		void fromglTfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler, GraphicsDevice* device, VkQueue copyQueue);
	};

	struct Material {
		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		glm::vec4 emissiveFactor = glm::vec4(0.0f);
		Texture2D* baseColorTexture;
		Texture2D* metallicRoughnessTexture;
		Texture2D* normalTexture;
		Texture2D* occlusionTexture;
		Texture2D* emissiveTexture;
		bool doubleSided = false;
		struct TexCoordSets {
			uint8_t baseColor = 0;
			uint8_t metallicRoughness = 0;
			uint8_t specularGlossiness = 0;
			uint8_t normal = 0;
			uint8_t occlusion = 0;
			uint8_t emissive = 0;
		} texCoordSets;
		struct Extension {
			Texture2D* specularGlossinessTexture;
			Texture2D* diffuseTexture;
			glm::vec4 diffuseFactor = glm::vec4(1.0f);
			glm::vec3 specularFactor = glm::vec3(0.0f);
		} extension;
		struct PbrWorkflows {
			bool metallicRoughness = true;
			bool specularGlossiness = false;
		} pbrWorkflows;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		int index = 0;
		bool unlit = false;
		float emissiveStrength = 1.0f;
	};

	struct Primitive {
		uint32_t first_index = 0;
		uint32_t index_count = 0;
		uint32_t vertex_count = 0;
		int material_index;
		bool has_indices = false;
		Primitive(uint32_t _first_index, uint32_t _index_count, uint32_t _vertex_count, int index)
			:first_index(_first_index), index_count(_index_count), vertex_count(_vertex_count), material_index(index) {}
	};

	struct Mesh {
		std::vector<Primitive*> primitives;
		glm::mat4 matrix;
		Mesh(const glm::mat4& mat)
			:matrix(mat) {}
	};

	struct Node {
		Node* parent;
		uint32_t index;
		std::vector<Node*> children;
		Mesh* mesh;
		glm::mat4 matrix;
		std::string name;
		glm::vec3 translation;
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation;
		//~�ode();
	};

	class Model {
	public:
		Model() {}
		void Load(const std::string& path, GraphicsDevice* device);
		void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, uint32_t& vertex_count, uint32_t& index_count);
		void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t node_index, const tinygltf::Model& model);
		void LoadMaterials(tinygltf::Model model);

		const std::vector<Node*> GetNodes() const { return m_nodes; }
		const std::vector<Node*> GetLinearNodes() const { return m_linear_nodes; }
		const std::vector<Material>& GetMaterials() const { return m_materials; }
		const Material& GetMaterial(int i) const { return m_materials[i]; }
		Material& GetMaterial(int i) { return m_materials[i]; }
	private:
		std::vector<Node*> m_nodes;
		std::vector<Node*> m_linear_nodes;
		std::vector<Texture2D*> m_textures;
		std::vector<TextureSampler> m_texture_samplers;
		std::vector<Material> m_materials;
		uint32_t* m_index_buffer;
		Vertex* m_vertex_buffer;
		uint32_t m_vertex_pos = 0;
		uint32_t m_index_pos = 0;

	public:
		struct {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} m_vertices, m_indices;
	};
}