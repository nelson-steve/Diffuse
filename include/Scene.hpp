#pragma once

#include "GraphicsDevice.hpp"
#include "Camera.hpp"

namespace Diffuse {
	struct SceneObect {
		Model model;

		glm::vec3 position;
		glm::vec3 scale;
		glm::vec3 rotation;

		bool render = true;
	};

	struct Skybox {
		bool render = true;
	};

	struct SceneCamera {

	};

	class Scene {
	public:
		Scene() {};

		void AddSceneObect(SceneObect obect);
	private:
		Camera m_scene_camera;
		std::vector<SceneObect> m_scene_objects;
	};
}