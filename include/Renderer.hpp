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
	private:
		MeshData m_mesh_data;
	};

	class Renderer {
	public:
		Renderer(GraphicsDevice* graphics_device);
		Renderer() = delete;

		void RenderMesh(const Mesh& mesh);
		void RenderMeshes(const std::vector<Mesh>& meshes);
	private:
		GraphicsDevice* device;
	};
}