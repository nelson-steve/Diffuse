#include "tiny_gltf.h"

#include "GraphicsDevice.hpp"
#include "Texture2D.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include <iostream>

namespace Diffuse {

	struct Vertex;

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
			// We also store (and create) a descriptor set that's used to access this texture from the fragment shader
			VkDescriptorSet descriptorSet;
		};

		// A glTF texture stores a reference to the image and a sampler
		// In this sample, we are only interested in the image
		struct Texture {
			int32_t imageIndex;
		};

		std::vector<Image> images;
		Image image;
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

		/*
			glTF rendering functions
		*/

		// Draw a single node including child nodes (if present)
		//void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node* node)
		//{
		//	if (node->mesh.primitives.size() > 0) {
		//		// Pass the node's matrix via push constants
		//		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		//		glm::mat4 nodeMatrix = node->matrix;
		//		VulkanglTFModel::Node* currentParent = node->parent;
		//		while (currentParent) {
		//			nodeMatrix = currentParent->matrix * nodeMatrix;
		//			currentParent = currentParent->parent;
		//		}
		//		// Pass the final matrix to the vertex shader using push constants
		//		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
		//		for (VulkanglTFModel::Primitive& primitive : node->mesh.primitives) {
		//			if (primitive.indexCount > 0) {
		//				// Get the texture index for this primitive
		//				VulkanglTFModel::Texture texture = textures[materials[primitive.materialIndex].baseColorTextureIndex];
		//				// Bind the descriptor for the current primitive's texture
		//				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &images[texture.imageIndex].descriptorSet, 0, nullptr);
		//				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
		//			}
		//		}
		//	}
		//	for (auto& child : node->children) {
		//		drawNode(commandBuffer, pipelineLayout, child);
		//	}
		//}

		// Draw the glTF scene starting at the top-level-nodes
		//void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
		//{
		//	// All vertices and indices are stored in single buffers, so we only need to bind once
		//	VkDeviceSize offsets[1] = { 0 };
		//	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
		//	vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		//	// Render all nodes at top-level
		//	for (auto& node : nodes) {
		//		drawNode(commandBuffer, pipelineLayout, node);
		//	}
		//}

	};
}