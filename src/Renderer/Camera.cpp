#include "Camera.hpp"

#include "GLFW/glfw3.h"

namespace Diffuse {
	Camera::Camera() {
		m_position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_aspect = 1280.0f / 720.0f;
		m_near = 0.1f;
		m_far = 10000.0f;
		m_front = glm::vec3(0.0, 0.0, 1.0);
		m_up = glm::vec3(0.0, 1.0, 0.0);
		m_right = glm::vec3(1.0, 0.0, 0.0);
		m_sensitivity = 0.5f;
		m_speed = 2.5f;
		m_yaw = 0.0f;
		m_pitch = 0.0f;

		m_projection = glm::perspective(45.0f, m_aspect, m_near, m_far);
		m_view = glm::lookAt(m_position, m_position + m_front, m_up);
	}

	void Camera::Update(float dt, GLFWwindow* window) {
		dt = 1.0f;
		bool is_update = false;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			m_position -= m_right * (m_speed * dt);
			is_update = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			is_update = true;
			m_position += m_right * (m_speed * dt);
		}
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			is_update = true;
			m_position -= m_front * (m_speed * dt);
		}
		else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			is_update = true;
			m_position += m_front * (m_speed * dt);
		}
		//else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		//	m_position += m_up * (m_speed * dt);
		//}
		//else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		//	m_position -= m_up * (m_speed * dt);
		//}
		else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			is_update = true;
			m_pitch += m_sensitivity * dt;
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			is_update = true;
			m_pitch -= m_sensitivity * dt;
		}
		else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			is_update = true;
			m_yaw -= m_sensitivity * dt;
		}
		else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			is_update = true;
			m_yaw += m_sensitivity * dt;
		}
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			is_update = true;
			m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_front = glm::vec3(0.0, 0.0, 1.0);
			m_up = glm::vec3(0.0, 1.0, 0.0);
			m_right = glm::vec3(1.0, 0.0, 0.0);
		}

		if (is_update) {
			glm::vec3 front;
			front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
			front.y = sin(glm::radians(m_pitch));
			front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
			m_front = glm::normalize(front);
			// also re-calculate the Right and Up vector
			m_right = glm::normalize(glm::cross(m_front, m_up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
			m_up = glm::normalize(glm::cross(m_right, m_front));
			m_view = glm::lookAt(m_position, m_position + m_front, m_up);
		}
		//m_front = glm::normalize(glm::rotate(glm::quat(m_front), glm::vec3(m_rotation.x, m_rotation.y, m_rotation.z)));
		//m_projection = glm::perspective(45.0f, m_aspect, m_near, m_far);
	}
}