#pragma once

#include "tiny_gltf.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan.h"
#include <iostream>

namespace Diffuse {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv0;
		glm::vec2 uv1;
		glm::vec4 joint0;
		glm::vec4 weight0;
		glm::vec4 color;
	};

	struct Node {
		Node* parent;
		uint32_t index;
		std::vector<Node*> children;
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
		void Load(const std::string& path);
		void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model);
		void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t node_index, const tinygltf::Model& model);
	private:
		uint32_t* m_index_buffer;
		Vertex* m_vertex_buffer;
		uint32_t m_vertex_count = 0;
		uint32_t m_index_count = 0;

		struct {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} m_vertices, m_indices;
	};
}