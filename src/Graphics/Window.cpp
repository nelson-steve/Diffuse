#include "Window.hpp"

#include <assert.h>

namespace Diffuse {
	Window::Window() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(m_width, m_height, "Diffuse", nullptr, nullptr);
		assert(m_window);
	}

	void Window::DestroyWindow() {
		glfwDestroyWindow(m_window);
	}

	bool Window::WindowShouldClose()
	{
		return glfwWindowShouldClose(m_window);
	}
	void Window::PollEvents()
	{
		glfwPollEvents();
	}
}