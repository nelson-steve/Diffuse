#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Diffuse {	
	class Window {
	public:
		Window();
		void DestroyWindow();

		bool WindowShouldClose();
		inline void WindowResized(bool resized) { m_window_resized = resized; }
		inline bool IsWindowResized() const { return m_window_resized; }
		void PollEvents();

		inline void SetWidth(uint32_t width)  { m_width = width;   }
		inline void SetHeight(uint32_t height) { m_height = height; }

		GLFWwindow* window() const { return m_window; }
	private:
		GLFWwindow* m_window = nullptr;
		uint32_t m_width = 1280;
		uint32_t m_height = 720;
		bool m_window_resized = false;
	};
}