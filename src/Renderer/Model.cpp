#include "Model.hpp"

#include "GraphicsDevice.hpp"
#include "Texture2D.hpp"

#include "ktx.h"
#include "ktxvulkan.h"

namespace Diffuse {
	void Model::Load(const std::string& path) {
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

		if (file_loaded) {
			// TODO: Load material data
			const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
			for (auto& node_index : scene.nodes) {
				GetNodeProps(model.nodes[node_index], model);
			}
			assert(m_vertex_count > 0);
			m_vertex_buffer = new Vertex[m_vertex_count];
			m_index_buffer = new uint32_t[m_index_count];

			for (auto& node_index : scene.nodes) {
				const tinygltf::Node node = model.nodes[node_index];
				LoadNode(nullptr, node, node_index, model);
			}
		}
	}

	void Model::GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model) {
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				GetNodeProps(model.nodes[node.children[i]], model);
			}
		}
		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				auto primitive = mesh.primitives[i];
				m_vertex_count += model.accessors[primitive.attributes.find("POSITION")->second].count;
				if (primitive.indices > -1) {
					m_index_count += model.accessors[primitive.indices].count;
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
			
		}
	}
}