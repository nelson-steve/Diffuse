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

	class EditorCamera {
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip, GLFWwindow* window);

		void OnUpdate(float t, GLFWwindow* window);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline void SetMouseOffset(float xOffset, float yOffset) { m_MouseOffset = glm::vec2(xOffset, yOffset); }
		bool OnMouseScroll(float yOffset);

		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }
		inline void MouseMoved() { m_MouseMoved = true; }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }
		glm::mat4 GetProjection() const { return m_Projection; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
	private:
		void UpdateProjection();
		void UpdateView();

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		bool m_MouseMoved = false;
		glm::vec2 m_MouseOffset{ 0.0f };
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 10000.0f;

		glm::mat4 m_Projection;
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};
}