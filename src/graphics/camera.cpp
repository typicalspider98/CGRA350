#include "camera.h"
#include "../main/constants.h"


Camera::Camera() : m_position(0.0f, 0.0f, 0.0f), m_azimuthal_angle(-135.0f), m_polar_angle(-10.0f), m_resolution(500)
{

}

Camera::Camera(glm::vec3 position, int resolution) : m_position(position), m_azimuthal_angle(-135.0f), m_polar_angle(-10.0f), m_resolution(resolution)
{

}

Camera::Camera(glm::vec3 position, float azimuthal_angle, float polar_angle, int resolution)
	: m_position(position), m_azimuthal_angle(azimuthal_angle), m_polar_angle(polar_angle), m_resolution(resolution)
{

}

// Move camera's position in given direction, based on time taken & cam's velocity.
void Camera::move(CameraMovement direction, float delta_time)
{
	float distance = delta_time * m_speed;

	switch (direction)
	{
		case CameraMovement::FORWARDS:
		{
			// move along 'front' direction, but horizontally in x-z plane only
			glm::vec3 front = getFrontVector();
			m_position += glm::normalize(glm::vec3(front.x, 0.0f, front.z)) * distance;
			break;
		}
		case CameraMovement::BACKWARDS:
		{
			// move along '-front' direction, but horizontally in x-z plane only
			glm::vec3 front = getFrontVector();
			m_position -= glm::normalize(glm::vec3(front.x, 0.0f, front.z)) * distance;
			break;
		}
		case CameraMovement::LEFT:
		{
			// move along '-right' (= cross product of 'up' & 'front') direction
			glm::vec3 right = getRightVector();
			m_position -= glm::normalize(glm::vec3(right.x, 0.0f, right.z)) * distance;
			break;
		}
		case CameraMovement::RIGHT:
		{
			// move along 'right' (= cross product of 'up' & 'front') direction
			glm::vec3 right = getRightVector();
			m_position += glm::normalize(glm::vec3(right.x, 0.0f, right.z)) * distance;
			break;
		}
		case CameraMovement::UPWARDS:
		{
			// move along 'up' direction
			glm::vec3 up = getUpVector();
			m_position += up * distance;
			break;
		}
		case CameraMovement::DOWNWARDS:
		{
			// move along '-up' direction
			glm::vec3 up = getUpVector();
			m_position -= up * distance;
			break;
		}
		default:
			break;
	}

	SetIsMoving(true);
}

// Process mouse movement (drag & move). Update direction of camera.
void Camera::processMouseMovement(float x_offset, float y_offset)
{
	x_offset *= m_SENSITIVITY;
	y_offset *= m_SENSITIVITY;

	m_azimuthal_angle += x_offset;
	m_polar_angle += y_offset;

	// limit movement
	//m_azimuthal_angle = glm::clamp(m_azimuthal_angle, -180.0f, -60.0f); // -140.0f, -90.0f
	//m_polar_angle = glm::clamp(m_polar_angle, -45.0f, 15.0f);  // -30.0f, 0.0f)

	SetIsMoving(true);
}

// Process mouse scroll. Update FOV for zoom effect
void Camera::processMouseScroll(float scroll_amt)
{
	m_fov -= scroll_amt;
	m_fov = glm::clamp(m_fov, 1.0f, 45.0f);

	SetIsMoving(true);
}

// Setters
void Camera::setPosition(const glm::vec3 &new_pos)
{
	m_position = new_pos;
	SetIsMoving(true);
}

void Camera::setAzimuthalAngle(float new_azimuthal)
{
	m_azimuthal_angle = new_azimuthal;
}

void Camera::setPolarAngle(float new_polar)
{
	m_polar_angle = new_polar;
}

void Camera::setFOV(float new_fov)
{
	m_fov = new_fov;
}

void Camera::setSpeed(float new_speed)
{
	m_speed = new_speed;
}

void Camera::setAspectRatio(float new_ar)
{
	m_aspect_ratio = new_ar;
}

// Getters
glm::vec3 Camera::getPosition() const
{
	return m_position;
}

float Camera::getAzimuthalAngle() const
{
	return m_azimuthal_angle;
}

float Camera::getPolarAngle() const
{
	return m_polar_angle;
}

float Camera::getFOV() const
{
	return m_fov;
}

float Camera::getSpeed() const
{
	return m_speed;
}

glm::vec3 Camera::getFrontVector() const
{
	return glm::normalize(glm::vec3(
		glm::cos(glm::radians(m_polar_angle)) * glm::cos(glm::radians(m_azimuthal_angle)),
		glm::sin(glm::radians(m_polar_angle)),
		glm::cos(glm::radians(m_polar_angle)) * glm::sin(glm::radians(m_azimuthal_angle))
	));
}

glm::vec3 Camera::getRightVector() const
{
	return glm::normalize(cross({ 0,1,0 }, getFrontVector()));
}

glm::vec3 Camera::getUpVector() const
{
	return glm::normalize(cross(getFrontVector(), getRightVector()));
}

// Calc View matrix using LookAt
glm::mat4 Camera::getViewMatrix() const
{
	glm::mat4 view_matrix;
	this->getViewMatrix(view_matrix);

	return view_matrix;
}

void Camera::getViewMatrix(glm::mat4 &dest) const
{
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 target = m_position + getFrontVector();
	dest = glm::lookAt(m_position, target, up);
}

// Calc Projection matrix
glm::mat4 Camera::getProjMatrix() const
{
	glm::mat4 proj_matrix;
	this->getProjMatrix(proj_matrix);

	return proj_matrix;
}

void Camera::getProjMatrix(glm::mat4 &dest) const
{
	dest = glm::perspective(glm::radians(m_fov), m_aspect_ratio, CGRA350Constants::CAMERA_NEAR_PLANE, CGRA350Constants::CAMERA_FAR_PLANE);
}

int Camera::GetResolution() const
{
	return m_resolution;
}

void Camera::SetIsMoving(bool isMoving)
{
	this->m_isMoving = isMoving;
}

bool Camera::GetIsMoving() const
{
	return this->m_isMoving;
}