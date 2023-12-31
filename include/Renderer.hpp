#pragma once

#include "GraphicsDevice.hpp"
#include "Camera.hpp"

namespace Diffuse {
	class Renderer {
	public:
		Renderer(GraphicsDevice* graphics_device);
		Renderer() = delete;

		void RenderScene(std::shared_ptr<Scene> scene, Camera* camera, float dt);
		void RenderModel(Camera* camera, float dt, Model* model);
	private:
		GraphicsDevice* device;
	};
}