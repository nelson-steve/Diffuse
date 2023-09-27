#pragma once

#include "GraphicsDevice.hpp"

namespace Diffuse {
	class Renderer {
	public:
		Renderer(GraphicsDevice* graphics_device);
		Renderer() = delete;

		void RenderModel(Model* model);
	private:
		GraphicsDevice* device;
	};
}