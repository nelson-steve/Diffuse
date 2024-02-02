#pragma once

#include "Camera.hpp"
#include "Model.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace Diffuse {
	struct SceneObject {
		//SceneObect(const std::string& path) {};
		Model p_model;

		struct transform{
		public:
			const glm::mat4& get() { return mat; }
			void set_position(const glm::vec3& pos) { m_position = pos; update(); }
			void set_scale(const glm::vec3& scale) { m_scale = scale; update(); }
			void set_rotation(const glm::vec3& rot) { m_rotation = rot; update(); }
		private:
			void update() {
				glm::mat4 rotation = glm::toMat4(glm::quat(m_rotation));
				mat = glm::translate(glm::mat4(1.0f), m_position) * rotation * glm::scale(glm::mat4(1.0f), m_scale);
			}
		private:
			glm::vec3 m_position{0.0f};
			glm::vec3 m_scale{1.0f};
			glm::vec3 m_rotation{0.0f};

			glm::mat4 mat{1.0f};
		} p_transform;

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
		std::shared_ptr<Camera> p_camera;
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
		//void AddSceneCamera(const std::shared_ptr<SceneCamera> camera) { m_scene_camera = camera; }
		void AddEditorCamera(const std::shared_ptr<EditorCamera> camera) { m_editor_camera = camera; }

		std::shared_ptr<SceneCamera> GetSceneCamera() { return m_scene_camera; }
		std::vector<std::shared_ptr<SceneObject>> GetSceneObjects() const { return m_scene_objects; }
		std::shared_ptr<Skybox> GetSkybox() const { return m_skybox; }
	private:
		std::shared_ptr<SceneCamera> m_scene_camera;
		std::shared_ptr<EditorCamera> m_editor_camera;
		std::shared_ptr<Skybox> m_skybox;
		std::vector<std::shared_ptr<SceneObject>> m_scene_objects;
	};
}