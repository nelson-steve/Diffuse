#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Diffuse {	
	class Window {
	private:
		GLFWwindow* m_window = nullptr;
		uint32_t m_width = 1280;
		uint32_t m_height = 720;
	public:
		Window();
		void DestroyWindow();

		bool WindowShouldClose();

		void PollEvents();

		void SetWidth(uint32_t width)  { m_width = width;   }
		void SetHeight(uint32_t height) { m_height = height; }

		GLFWwindow* window() const { return m_window; }
	};
}