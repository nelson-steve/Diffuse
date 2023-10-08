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
			Texture2D* base_color_texture;
			Texture2D* metallic_roghness_texture;
			Texture2D* normal_texture;
			Texture2D* occlusion_texture;
			Texture2D* emissive_texture;
			Texture2D* base_color_texture;

			glm::vec4 base_color_factor = glm::vec4(1.0f);
			float metallic_factor = 1.0f;
			float roughness_factor = 0.0f;
			glm::vec3 emissive_factor = glm::vec3(1.0f);
			uint32_t baseColorTextureIndex;
			float alpha_cutt_off = 0.5f;

			struct TexCoordSets {
				uint8_t baseColor = 0;
				uint8_t metallicRoughness = 0;
				uint8_t specularGlossiness = 0;
				uint8_t normal = 0;
				uint8_t occlusion = 0;
				uint8_t emissive = 0;
			} tex_coord_sets;

			enum {
				ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND
			} alpha_mode = ALPHAMODE_OPAQUE;
		};

		struct Image {
			Texture2D* texture;
			VkDescriptorSet descriptorSet;
		};

		struct TextureSampler {
			VkFilter magFilter;
			VkFilter minFilter;
			VkSamplerAddressMode addressModeU;
			VkSamplerAddressMode addressModeV;
			VkSamplerAddressMode addressModeW;
		};

		std::vector<TextureSampler> textureSamplers;
		std::vector<Texture2D*> textures;
		std::vector<Material> materials;
		std::vector<Node*> nodes;
		uint32_t p_indices_size;

		GraphicsDevice* m_graphics_device;

		Model() = delete;
		Model(const std::string& path, GraphicsDevice* graphics_device);
		~Model();
		void LoadTextures(tinygltf::Model& input);
		void LoadMaterials(tinygltf::Model& input);
		void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Model::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	};
}