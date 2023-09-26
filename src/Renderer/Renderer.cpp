#include "Renderer.hpp"

#include "tiny_obj_loader.h"
#include <iostream>
#include <unordered_map>

namespace Diffuse {
	Mesh::Mesh(const std::string& path) {
		// loading a model
        std::string inputfile = path;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = "assets/"; // Path to material files

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(inputfile, reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            exit(1);
        }

        if (!reader.Warning().empty()) {
            std::cout << "TinyObjReader: " << reader.Warning();
        }

        std::cout << "Started loading model" << std::endl;
        
        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    Vertex vertex{};
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    vertex.pos = {
                        attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]
                    };
                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    //if (idx.normal_index >= 0) {
                    //    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    //    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    //    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    //}

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0) {
                        tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                        
                        vertex.tex_coords = { tx, ty };
                    }

                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(m_mesh_data.vertices.size());
                        m_mesh_data.vertices.push_back(vertex);
                    }

                    m_mesh_data.indices.push_back(uniqueVertices[vertex]);

                    // Optional: vertex colors
                    // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                    // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                    // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }
        std::cout << "Done loading model: " << path << std::endl;
	}

	Renderer::Renderer(GraphicsDevice* graphics_device) {
        device = graphics_device;
	}

	void Renderer::RenderMesh(Mesh& mesh) {
        //device->Draw(mesh);
	}

	void Renderer::RenderMeshes(std::vector<Mesh*> meshes) {

        for (auto& mesh : meshes) {
            if (!mesh->is_initialized) {
                device->CreateDescriptorSet(*mesh);
                device->CreateVertexBuffer(*mesh, mesh->GetMeshData().vertices);
                mesh->p_indices_size = mesh->GetMeshData().indices.size();
                device->CreateIndexBuffer(*mesh, mesh->GetMeshData().indices);

                mesh->is_initialized = true;
            }
        }
        std::vector<Mesh> _meshes;
        for (auto& mesh : meshes) {
            _meshes.push_back(*mesh);
        }
        device->Draw(_meshes);
	}
}