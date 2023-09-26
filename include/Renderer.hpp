#pragma once

#include "GraphicsDevice.hpp"

namespace Diffuse {
	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::string texture_path;
	};

	class Mesh {
	public:
		Mesh(const std::string& path);
		Mesh() = delete;

		void SetTexturePath(const std::string& path) { m_mesh_data.texture_path = path; }

		const MeshData& GetMeshData() const { return m_mesh_data; }
		mutable bool is_initialized = false;

		int p_indices_size = 0;
		VkBuffer p_index_buffer;
		VkBuffer p_vertex_buffer;
		//VkPipeline p_graphics_pipeline;
		VkDeviceMemory p_vertex_buffer_memory;
		VkDeviceMemory p_index_buffer_memory;
		//VkPipelineLayout p_pipeline_layout;
		//VkDescriptorSetLayout p_descriptor_set_layout;
		//VkDescriptorPool p_descriptor_pool;
		//std::vector<VkDescriptorSet> p_descriptor_sets;
	private:
		MeshData m_mesh_data;
	};

	class Renderer {
	public:
		Renderer(GraphicsDevice* graphics_device);
		Renderer() = delete;

		void RenderMesh(Mesh& mesh);
		void RenderMeshes(std::vector<Mesh*> meshes);
	private:
		GraphicsDevice* device;
	};
}