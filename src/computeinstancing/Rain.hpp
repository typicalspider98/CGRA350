#pragma once

#include "../graphics/camera.h"
#include "../graphics/textures.h"
#include "../graphics/shaders.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

struct Raindrop {
	glm::vec4 position;
	glm::vec4 velocity;
};

struct Splash {
	glm::vec4 position;
	glm::vec4 lifetime;  // x-total lifetime; y-left lifetime
};

class Rain
{
private:
	ShaderProgram m_computeShader;
	ShaderProgram m_raindropShader;
	ShaderProgram m_splashShader;

	GLuint m_raindrop_num;
	std::vector<Raindrop> m_raindrops;
	GLuint m_splash_max;
	std::vector<Splash> m_splashes;
#if IRIS_DEBUG
	std::vector<glm::vec4> m_debugs;
#endif

	GLuint m_raindrop_vao, m_raindrop_ssbo;
	GLuint m_splash_vao, m_splash_vbo, m_splash_ssbo;
	GLuint m_debug_ssbo;

	Texture2D m_splash_texture;

	void setupShadersAndBuffers();

public:
	Rain(ShaderProgram& computeShader, ShaderProgram& raindropShader, ShaderProgram& splashShader, const Texture2D& splash_texture);

	void clearRain();
	void initializeRain(int numDrops, const glm::vec3& rainPosition, float cloudRadius, float minSpeed, float maxSpeed, float seaLevel);
	glm::vec4 generateRainDropPosition(const glm::vec3& rainPosition, float cloudRadius, float seaLevel);
	glm::vec4 generateRainDropVelocity(float minSpeed, float maxSpeed);

	void computeRainOnGPU(float deltaTime, float seaLevel, const glm::vec3& rainPosition, float cloudRadius, float minSpeed, float maxSpeed);
	void renderRaindrops(const glm::mat4& projection, const glm::mat4& view, float deltaTime, float raindrop_size, const glm::vec3& raindrop_color);
	void renderSplashes(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraRight, const glm::vec3& cameraUp, float deltaTime);
};