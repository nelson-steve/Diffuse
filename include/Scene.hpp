#pragma once

#include "GraphicsDevice.hpp"
#include "Camera.hpp"

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
		VkDescriptorSetLayout desc_set_layout;

		struct {
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;
		} p_ubo;
	};

	struct Skybox {
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
		void AddSceneCamera(const std::shared_ptr<SceneCamera> camera) { m_scene_camera = camera; }

		std::vector<std::shared_ptr<SceneObject>> GetSceneObjects() const { return m_scene_objects; }
	private:
		std::shared_ptr<SceneCamera> m_scene_camera;
		std::shared_ptr<Skybox> m_skybox;
		std::vector<std::shared_ptr<SceneObject>> m_scene_objects;
	};
}