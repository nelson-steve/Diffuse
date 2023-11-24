#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

struct GLFWwindow;

namespace Diffuse {
	class Camera {
	public:
		Camera();

		void Update(float dt, GLFWwindow* window);

		void SetPosition(const glm::vec3& position);

		glm::mat4 GetView() {
			return m_view;
		}
		glm::mat4 GetProjection() const {
			return m_projection;
		}
		glm::vec3 GetPosition() const {
			return m_position;
		}
	private:
		glm::vec3 m_position;
		glm::vec3 m_front;
		glm::vec3 m_right;
		glm::vec3 m_up;
		glm::mat4 m_projection;
		glm::mat4 m_view;
		float m_yaw;
		float m_pitch;
		float m_speed;
		float m_sensitivity;
		float m_aspect;
		float m_fov;
		float m_near;
		float m_far;
		bool m_is_update = true;
	};
}