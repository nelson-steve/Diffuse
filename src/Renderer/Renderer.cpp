#include "Renderer.hpp"

#include "tiny_gltf.h"
#include <iostream>
#include <unordered_map>

namespace Diffuse {
	Renderer::Renderer(GraphicsDevice* graphics_device)
		:device(graphics_device) { }

	//void Renderer::RenderModel(Camera* camera, float dt, Model* model) {
    //    //device->Draw(camera, model);
	//}
	//
	void Renderer::RenderScene(const std::shared_ptr<Scene> scene, std::shared_ptr<EditorCamera> camera, float dt) {
		device->Draw(scene, camera, dt);
	}
}