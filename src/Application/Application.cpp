#include "Application.hpp"

#include <iostream>

namespace Diffuse {
	void Application::Init() {
		m_graphics = new Graphics();
		m_config.enable_validation_layers = false;
		if (m_graphics->Init(m_config)) {
			std::cout << "SUCCESS";
		}
		else {
			std::cout << "Couldn't create Vulkan Instance";
		}
	}
}