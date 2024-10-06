#pragma once

#include "../graphics/camera.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

class Rain
{
private:
	GLuint m_computeShader;
	GLuint m_renderShader;

	std::vector<glm::vec4> m_positions;
	std::vector<glm::vec4> m_velocities;
#if IRIS_DEBUG
	std::vector<glm::vec4> m_debugs;
#endif

	GLuint m_vao, m_vbo_positions;
	GLuint m_ssbo_positions, m_ssbo_velocities, m_ssbo_debug;

public:
	Rain(GLuint computeShader, GLuint renderShader);

	void clearRain();
	void initializeRain(int numDrops, const glm::vec3& cloudPosition, float cloudRadius, float minSpeed, float maxSpeed);
	glm::vec4 generateRainDropPosition(const glm::vec3& cloudPosition, float cloudRadius);
	glm::vec4 generateRainDropVelocity(float minSpeed, float maxSpeed);

	void computeRainOnGPU(float deltaTime, float seaLevel, const glm::vec3& cloudPosition, float cloudRadius, float minSpeed, float maxSpeed);
	void renderRain(const Camera& camera, float deltaTime);
};