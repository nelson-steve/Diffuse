#include "Model.hpp"

namespace Diffuse {
	Model::Model(const std::string& path, GraphicsDevice* graphics_device) {
		m_graphics_device = graphics_device;
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

		if (!warn.empty()) {
			printf("Warn: %s\n", warn.c_str());
		}

		if (!err.empty()) {
			printf("ERR: %s\n", err.c_str());
		}
		if (!ret) {
			printf("Failed to load .glTF : %s\n", path);
			assert(false);
		}

		std::vector<uint32_t> indexBuffer;
		std::vector<Vertex> vertexBuffer;

		tinygltf::Scene scene = model.scenes[model.defaultScene];
		int size = scene.nodes.size();
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = model.nodes[scene.nodes[i]];
			LoadNode(node, model, nullptr, indexBuffer, vertexBuffer);
		}

		size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
		size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		p_indices_size = indexBuffer.size();
		p_indices.count = static_cast<uint32_t>(indexBuffer.size());

		// Vertex Buffer
		{
			VkDeviceSize bufferSize = sizeof(vertexBuffer[0]) * vertexBuffer.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

			void* data;
			vkMapMemory(m_graphics_device->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, vertexBuffer.data(), (size_t)bufferSize);
			vkUnmapMemory(m_graphics_device->m_device, stagingBufferMemory);

			vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_vertices.buffer, p_vertices.memory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

			vkUtilities::CopyBuffer(stagingBuffer, p_vertices.buffer, bufferSize, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_graphics_device->m_graphics_queue);

			vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
			vkFreeMemory(m_graphics_device->m_device, stagingBufferMemory, nullptr);
		}

		// Index Buffer
		{
			VkDeviceSize bufferSize = sizeof(indexBuffer[0]) * indexBuffer.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

			void* data;
			vkMapMemory(m_graphics_device->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, indexBuffer.data(), (size_t)bufferSize);
			vkUnmapMemory(m_graphics_device->m_device, stagingBufferMemory);

			vkUtilities::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_indices.buffer, p_indices.memory, m_graphics_device->m_physical_device, m_graphics_device->m_device);

			vkUtilities::CopyBuffer(stagingBuffer, p_indices.buffer, bufferSize, m_graphics_device->m_command_pool, m_graphics_device->m_device, m_graphics_device->m_graphics_queue);

			vkDestroyBuffer(m_graphics_device->m_device, stagingBuffer, nullptr);
			vkFreeMemory(m_graphics_device->m_device, stagingBufferMemory, nullptr);
		}

		LoadImages(model);
		//image.texture = new Texture2D("../assets/Avocado/Avocado_baseColor.png", m_graphics_device);
	}

	Model::~Model()
	{
		for (auto node : nodes) {
			delete node;
		}
		//vkDestroyBuffer(vulkanDevice->logicalDevice, vertices.buffer, nullptr);
		//vkFreeMemory(vulkanDevice->logicalDevice, vertices.memory, nullptr);
		//vkDestroyBuffer(vulkanDevice->logicalDevice, indices.buffer, nullptr);
		//vkFreeMemory(vulkanDevice->logicalDevice, indices.memory, nullptr);
	}

	void Model::LoadImages(tinygltf::Model& input)
	{
		// Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
		// loading them from disk, we fetch them from the glTF loader and upload the buffers
		images.resize(input.images.size());
		for (size_t i = 0; i < input.images.size(); i++) {
			tinygltf::Image& glTFImage = input.images[i];
			// Get the image data from the glTF loader
			unsigned char* buffer = nullptr;
			VkDeviceSize bufferSize = 0;
			bool deleteBuffer = false;
			// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
			if (glTFImage.component == 3) {
				bufferSize = glTFImage.width * glTFImage.height * 4;
				buffer = new unsigned char[bufferSize];
				unsigned char* rgba = buffer;
				unsigned char* rgb = &glTFImage.image[0];
				for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
					memcpy(rgba, rgb, sizeof(unsigned char) * 3);
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else {
				buffer = &glTFImage.image[0];
				bufferSize = glTFImage.image.size();
			}
			// Load texture from image buffer
			images[i].texture = new Texture2D(buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, glTFImage.width, glTFImage.height, m_graphics_device);
			if (deleteBuffer) {
				delete[] buffer;
			}
		}
	}

	void Model::LoadTextures(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());
		for (size_t i = 0; i < input.textures.size(); i++) {
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void Model::LoadMaterials(tinygltf::Model& input)
	{
		materials.resize(input.materials.size());
		for (size_t i = 0; i < input.materials.size(); i++) {
			// We only read the most basic properties required for our sample
			tinygltf::Material glTFMaterial = input.materials[i];
			// Get the base color factor
			if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
				materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}
			// Get base color texture index
			if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
				materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
			}
		}
	}

	void Model::LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Model::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
	{
		Node* node = new Node{};
		node->matrix = glm::mat4(1.0f);
		node->parent = parent;

		// Get the local node matrix
		// It's either made up from translation, rotation, scale or a 4x4 matrix
		if (inputNode.translation.size() == 3) {
			node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
		}
		if (inputNode.rotation.size() == 4) {
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node->matrix *= glm::mat4(q);
		}
		if (inputNode.scale.size() == 3) {
			node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
		}
		if (inputNode.matrix.size() == 16) {
			node->matrix = glm::make_mat4x4(inputNode.matrix.data());
		};

		// Load node's children
		if (inputNode.children.size() > 0) {
			for (size_t i = 0; i < inputNode.children.size(); i++) {
				LoadNode(input.nodes[inputNode.children[i]], input, node, indexBuffer, vertexBuffer);
			}
		}

		// If the node contains mesh data, we load vertices and indices from the buffers
		// In glTF this is done via accessors and buffer views
		if (inputNode.mesh > -1) {
			const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
			// Iterate through all primitives of this node's mesh
			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
				uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
				uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
				uint32_t indexCount = 0;
				// Vertices
				{
					const float* positionBuffer = nullptr;
					const float* normalsBuffer = nullptr;
					const float* texCoordsBuffer = nullptr;
					size_t vertexCount = 0;

					// Get buffer data for vertex positions
					if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						vertexCount = accessor.count;
					}
					// Get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}
					// Get buffer data for vertex texture coordinates
					// glTF supports multiple sets, we only load the first one
					if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// Append data to model's vertex buffer
					for (size_t v = 0; v < vertexCount; v++) {
						Vertex vert{};
						vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
						vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
						vert.tex_coords = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
						vert.color = glm::vec3(1.0f);
						vertexBuffer.push_back(vert);
					}
				}
				// Indices
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

					indexCount += static_cast<uint32_t>(accessor.count);

					// glTF supports different component types of indices
					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					default:
						std::cout << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
					}
				}
				Primitive primitive{};
				primitive.firstIndex = firstIndex;
				primitive.indexCount = indexCount;
				primitive.materialIndex = glTFPrimitive.material;
				node->mesh.primitives.push_back(primitive);
			}
		}

		if (parent) {
			parent->children.push_back(node);
		}
		else {
			nodes.push_back(node);
		}
	}
}