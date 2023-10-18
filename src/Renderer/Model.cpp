#include "Model.hpp"

#include "GraphicsDevice.hpp"
#include "Texture2D.hpp"

#include "ktx.h"
#include "ktxvulkan.h"

namespace Diffuse {
	void Model::Load(const std::string& path, GraphicsDevice* device) {
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string error;
		std::string warning;

		bool binary = false;
		size_t extpos = path.rfind('.', path.length());
		if (extpos != std::string::npos) {
			binary = (path.substr(extpos + 1, path.length() - extpos) == "glb");
		}

		bool file_loaded = false;
		if (binary) {
			file_loaded = loader.LoadBinaryFromFile(&model, &error, &warning, path.c_str());
		}
		else {
			file_loaded = loader.LoadASCIIFromFile(&model, &error, &warning, path.c_str());
		}

		uint32_t vertex_count = 0;
		uint32_t index_count = 0;
		if (file_loaded) {
			// TODO: Load material data
			// Load Textures
			for (tinygltf::Texture& tex : model.textures) {
				tinygltf::Image image = model.images[tex.source];
				TextureSampler texture_sampler;
				//if (tex.sampler == -1) 
				{
					// No sampler specified, use a default one
					texture_sampler.mag_filter = VK_FILTER_LINEAR;
					texture_sampler.min_filter = VK_FILTER_LINEAR;
					texture_sampler.address_modeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					texture_sampler.address_modeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					texture_sampler.address_modeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				}
				//else {
				//	textureSampler = textureSamplers[tex.sampler];
				//}
				Texture2D* texture;
				texture = new Texture2D(image, texture_sampler, device->Queue(), device);
				m_textures.push_back(texture);
			}

			const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
			for (auto& node_index : scene.nodes) {
				GetNodeProps(model.nodes[node_index], model, vertex_count, index_count);
			}
			assert(vertex_count > 0);
			m_vertex_buffer = new Vertex[vertex_count];
			m_index_buffer = new uint32_t[index_count];

			for (auto& node_index : scene.nodes) {
				const tinygltf::Node node = model.nodes[node_index];
				LoadNode(nullptr, node, node_index, model);
			}
		}

		size_t vertexBufferSize = vertex_count * sizeof(Vertex);
		size_t indexBufferSize = index_count * sizeof(uint32_t);
		assert(vertexBufferSize > 0);

		struct StagingBuffer {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertex_staging, index_staging;

		// Vertex buffer
		device->CreateVertexBuffer(m_vertices.buffer, m_vertices.memory, vertexBufferSize, m_vertex_buffer);
		// Index buffer
		if (indexBufferSize > 0) {
			device->CreateIndexBuffer(m_indices.buffer, m_indices.memory, indexBufferSize, m_index_buffer);
		}
	}

	void Model::GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, uint32_t& vertex_count, uint32_t& index_count) {
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				GetNodeProps(model.nodes[node.children[i]], model, vertex_count, index_count);
			}
		}
		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				auto primitive = mesh.primitives[i];
				vertex_count += model.accessors[primitive.attributes.find("POSITION")->second].count;
				if (primitive.indices > -1) {
					index_count += model.accessors[primitive.indices].count;
				}
			}
		}
	}

	void Model::LoadNode(Node* parent, const tinygltf::Node& node, uint32_t node_index, const tinygltf::Model& model) {
		Node* new_node = new Node();
		new_node->parent = parent;
		new_node->index = node_index;
		new_node->name = node.name;
		new_node->matrix = glm::mat4(1.0f);

		if (node.translation.size() == 3) {
			new_node->translation = glm::make_vec3(node.translation.data());
		}
		if (node.rotation.size() == 4) {
			new_node->rotation = glm::make_quat(node.rotation.data());
		}
		if (node.scale.size() == 4) {
			new_node->scale = glm::make_vec3(node.scale.data());
		}
		if (node.matrix.size() == 16) {
			new_node->matrix = glm::make_mat4x4(node.matrix.data());
		}

		if (node.children.size() > 0) {
			for (auto& node_index : node.children)
				LoadNode(new_node, model.nodes[node_index], node_index, model);
		}
		
		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			Mesh* new_mesh = new Mesh(new_node->matrix);
			for (auto& primitive : mesh.primitives) {
				uint32_t vertex_start = m_vertex_pos;
				uint32_t index_start = m_index_pos;
				uint32_t vertex_count = 0;
				uint32_t index_count = 0;
				// Vertices
				{
					const float* buffer_pos = nullptr;
					const float* buffer_normals = nullptr;
					const float* buffer_uv_set0 = nullptr;
					const float* buffer_uv_set1 = nullptr;
					const float* buffer_color_set0 = nullptr;
					const void*  buffer_joints = nullptr;
					const float* buffer_weights = nullptr;

					int posByteStride;
					int normByteStride;
					int uv0ByteStride;
					int uv1ByteStride;
					int color0ByteStride;
					int jointByteStride;
					int weightByteStride;

					if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
						const tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& pos_view = model.bufferViews[pos_accessor.bufferView];
						vertex_count = static_cast<uint32_t>(pos_accessor.count);
						buffer_pos = reinterpret_cast<const float*>(&(model.buffers[pos_view.buffer].data[pos_accessor.byteOffset + pos_view.byteOffset]));
						posByteStride = pos_accessor.ByteStride(pos_view) ? (pos_accessor.ByteStride(pos_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}
					else {
						assert(primitive.attributes.find("POSITION") != primitive.attributes.end());
					}

					if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
						const tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& normal_view = model.bufferViews[normal_accessor.bufferView];
						buffer_normals = reinterpret_cast<const float*>(&(model.buffers[normal_view.buffer].data[normal_accessor.byteOffset + normal_view.byteOffset]));
						normByteStride = normal_accessor.ByteStride(normal_view) ? (normal_accessor.ByteStride(normal_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& uv0_accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& uv0_view = model.bufferViews[uv0_accessor.bufferView];
						buffer_uv_set0 = reinterpret_cast<const float*>(&(model.buffers[uv0_view.buffer].data[uv0_accessor.byteOffset + uv0_view.byteOffset]));
						uv0ByteStride = uv0_accessor.ByteStride(uv0_view) ? (uv0_accessor.ByteStride(uv0_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
					}

					if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
						const tinygltf::Accessor& uv1_accessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
						const tinygltf::BufferView& uv1_view = model.bufferViews[uv1_accessor.bufferView];
						buffer_uv_set1 = reinterpret_cast<const float*>(&(model.buffers[uv1_view.buffer].data[uv1_accessor.byteOffset + uv1_view.byteOffset]));
						uv1ByteStride = uv1_accessor.ByteStride(uv1_view) ? (uv1_accessor.ByteStride(uv1_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
					}

					if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& color0_accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
						const tinygltf::BufferView& uv1_view = model.bufferViews[color0_accessor.bufferView];
						buffer_color_set0 = reinterpret_cast<const float*>(&(model.buffers[uv1_view.buffer].data[color0_accessor.byteOffset + uv1_view.byteOffset]));
						color0ByteStride = color0_accessor.ByteStride(uv1_view) ? (color0_accessor.ByteStride(uv1_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					const tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes.find("POSITION")->second];
					for (size_t v = 0; v < pos_accessor.count; v++) {
						Vertex& vert = m_vertex_buffer[m_vertex_pos];
						vert.pos = glm::vec4(glm::make_vec3(&buffer_pos[v * posByteStride]), 1.0f);
						vert.normal = glm::normalize(glm::vec3(buffer_normals ? glm::make_vec3(&buffer_normals[v * normByteStride]) : glm::vec3(0.0f)));
						vert.uv0 = buffer_uv_set0 ? glm::make_vec2(&buffer_uv_set0[v * uv0ByteStride]) : glm::vec3(0.0f);
						vert.uv1 = buffer_uv_set1 ? glm::make_vec2(&buffer_uv_set1[v * uv1ByteStride]) : glm::vec3(0.0f);
						vert.color = buffer_color_set0 ? glm::make_vec4(&buffer_color_set0[v * color0ByteStride]) : glm::vec4(1.0f);

						m_vertex_pos++;
					}

				}
				bool has_indices = primitive.indices > -1;
				if (has_indices) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
					const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

					index_count = static_cast<uint32_t>(accessor.count);
					const void* data_ptr = &(buffer.data[accessor.byteOffset + buffer_view.byteOffset]);

					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = static_cast<const uint32_t*>(data_ptr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_index_buffer[m_index_pos] = buf[index] + vertex_start;
							m_index_pos++;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = static_cast<const uint16_t*>(data_ptr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_index_buffer[m_index_pos] = buf[index] + vertex_start;
							m_index_pos++;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = static_cast<const uint8_t*>(data_ptr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_index_buffer[m_index_pos] = buf[index] + vertex_start;
							m_index_pos++;
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
					}
				}
				Primitive* new_primitive = new Primitive(index_start, index_count, vertex_count);
				new_mesh->primitives.push_back(new_primitive);
			}
			new_node->mesh = new_mesh;
		}
		if (parent) {
			parent->children.push_back(new_node);
		}
		else {
			m_nodes.push_back(new_node);
		}
		m_linear_nodes.push_back(new_node);
	}
}