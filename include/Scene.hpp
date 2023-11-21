#pragma once

#include "Camera.hpp"
#include "Model.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace Diffuse {
	struct SceneObject {
		//SceneObect(const std::string& path) {};
		Model p_model;

		glm::vec3 p_position{0.0f};
		glm::vec3 p_scale{1.0f};
		glm::vec3 p_rotation{0.0f};

		bool p_render = true;
		
		//
		VkDescriptorSet p_mat_descritpor_set;

		struct {
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;
		} p_ubo;

		struct {
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;
		} p_shader_values_ubo;

		struct {
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo descriptor;
		} p_shader_material_buffer;
	};

	struct Skybox {
		Model p_model;

		struct {
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;
		} p_ubo;

		bool p_render = true;
	};

	struct SceneCamera {
		Camera p_camera;
		glm::vec3 p_position;
		glm::vec3 p_scale;
		glm::vec3 p_rotation;
		float p_near = 0.1f;
		float p_far = 1000.0f;
		float p_aspect = 16.0/9.0f;
	};

	class Scene {
	public:
		void AddSceneObect(const std::shared_ptr<SceneObject> object) { m_scene_objects.push_back(object); }
		void AddSkybox(const std::shared_ptr<Skybox> skybox) { m_skybox = skybox; }
		void AddSceneCamera(const std::shared_ptr<SceneCamera> camera) { m_scene_camera = camera; }

		std::vector<std::shared_ptr<SceneObject>> GetSceneObjects() const { return m_scene_objects; }
		std::shared_ptr<Skybox> GetSkybox() const { return m_skybox; }
	private:
		std::shared_ptr<SceneCamera> m_scene_camera;
		std::shared_ptr<Skybox> m_skybox;
		std::vector<std::shared_ptr<SceneObject>> m_scene_objects;
	};
}