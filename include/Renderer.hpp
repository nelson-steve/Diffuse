#pragma once

#include "GraphicsDevice.hpp"

namespace Diffuse {
	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	class Mesh {
	public:
		Mesh(const std::string& path);
		Mesh() = delete;

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