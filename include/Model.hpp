#pragma once

#include "tiny_gltf.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan.h"
#include <iostream>

namespace Diffuse {

	class GraphicsDevice;
	class Texture2D;
	struct TextureSampler;

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


	struct Primitive {
		uint32_t first_index = 0;
		uint32_t index_count = 0;
		uint32_t vertex_count = 0;
		//Material& material;
		bool has_indices = false;
		Primitive(uint32_t _first_index, uint32_t _index_count, uint32_t _vertex_count)
			:first_index(_first_index), index_count(_index_count), vertex_count(_vertex_count) {}
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
		//~Ñode();
	};

	class Model {
	public:
		Model() {}
		void Load(const std::string& path, GraphicsDevice* device);
		void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, uint32_t& vertex_count, uint32_t& index_count);
		void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t node_index, const tinygltf::Model& model);

		const std::vector<Node*> GetNodes() const { return m_nodes; }
	private:
		std::vector<Node*> m_nodes;
		std::vector<Node*> m_linear_nodes;
		std::vector<Texture2D*> m_textures;
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