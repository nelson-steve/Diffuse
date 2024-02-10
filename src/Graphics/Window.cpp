#include "Window.hpp"

#include <assert.h>

namespace Diffuse {
	void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		w->WindowResized(true);
	}

	Window::Window() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		m_window = glfwCreateWindow(m_width, m_height, "Diffuse", nullptr, nullptr);
		assert(m_window);

		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
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