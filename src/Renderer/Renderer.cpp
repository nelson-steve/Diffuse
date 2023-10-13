#include "Renderer.hpp"

#include "tiny_gltf.h"
#include <iostream>
#include <unordered_map>

namespace Diffuse {
	Renderer::Renderer(GraphicsDevice* graphics_device) {
        device = graphics_device;
	}

	void Renderer::RenderModel(Camera* camera, float dt, Model* model) {
        //device->Draw(camera, model);
	}
}