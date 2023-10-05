#pragma once

#include "tiny_gltf.h"

#include "Texture2D.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include <iostream>

namespace Diffuse {

	struct Vertex;
	class GraphicsDevice;

	class Model {
	public:
		struct {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} p_vertices;

		struct {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} p_indices;

		struct Node;

		struct Primitive {
			uint32_t firstIndex;
			uint32_t indexCount;
			int32_t materialIndex;
		};

		struct Mesh {
			std::vector<Primitive> primitives;
		};

		struct Node {
			Node* parent;
			std::vector<Node*> children;
			Mesh mesh;
			glm::mat4 matrix;
			~Node() {
				for (auto& child : children) {
					delete child;
				}
			}
		};

		struct Material {
			glm::vec4 baseColorFactor = glm::vec4(1.0f);
			uint32_t baseColorTextureIndex;
		};

		struct Image {
			Texture2D* texture;
			VkDescriptorSet descriptorSet;
		};

		struct Texture {
			int32_t imageIndex;
		};

		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<Node*> nodes;
		uint32_t p_indices_size;

		GraphicsDevice* m_graphics_device;

		Model() = delete;
		Model(const std::string& path, GraphicsDevice* graphics_device);
		~Model();

		void LoadImages(tinygltf::Model& input);

		void LoadTextures(tinygltf::Model& input);

		void LoadMaterials(tinygltf::Model& input);

		void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Model::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	};
}