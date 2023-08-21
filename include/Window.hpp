#pragma once

class GLFWwindow;

namespace Diffuse {

	class Window {
	private:
		GLFWwindow* m_window = nullptr;
		uint32_t m_width;
		uint32_t m_height;
	public:
		Window();
		~Window();

		bool WindowShouldClose();

		void PollEvents();

		void SetWidth(uint32_t width)  { m_width = width;   }
		void SetWidth(uint32_t height) { m_height = height; }

		GLFWwindow* window() const { return m_window; }
	};
}