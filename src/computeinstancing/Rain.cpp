#include "Rain.hpp"
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Rain::Rain(GLuint computeShader, GLuint renderShader) : m_computeShader(computeShader), m_renderShader(renderShader)
{
    m_vao = 0;
    m_vbo_positions = 0;
    m_ssbo_positions = 0;
    m_ssbo_velocities = 0;
}

void Rain::clearRain()
{
    m_positions.clear();
    m_velocities.clear();

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo_positions) {
        glDeleteBuffers(1, &m_vbo_positions);
        m_vbo_positions = 0;
    }
    if (m_ssbo_positions) {
        glDeleteBuffers(1, &m_ssbo_positions);
        m_ssbo_positions = 0;
    }
    if (m_ssbo_velocities) {
        glDeleteBuffers(1, &m_ssbo_velocities);
        m_ssbo_velocities = 0;
    }

#if IRIS_DEBUG
    m_debugs.clear();
    if (m_ssbo_debug) {
        glDeleteBuffers(1, &m_ssbo_debug);
        m_ssbo_debug = 0;
    }
#endif
}

void Rain::initializeRain(int numDrops, const glm::vec3& cloudPosition, float cloudRadius, float minSpeed, float maxSpeed)
{
    for (int i = 0; i < numDrops; i++) 
    {
        glm::vec4 position = generateRainDropPosition(cloudPosition, cloudRadius);
        glm::vec4 velocity = generateRainDropVelocity(minSpeed, maxSpeed);
        m_positions.push_back(position);
        m_velocities.push_back(velocity);
#if IRIS_DEBUG
        m_debugs.push_back(glm::vec4(0, 0, 0, 0));

        printf("cpu rain drop (%d). position: (%f, %f, %f), velocity: (%f, %f, %f)\n", i, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
#endif
    }

    // generate and bind vao
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // generate and bind vbo_position
    glGenBuffers(1, &m_vbo_positions);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
    glBufferData(GL_ARRAY_BUFFER, numDrops * sizeof(glm::vec4), m_positions.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glVertexAttribDivisor(0, 1);

    // generate and bind ssbo_position
    glGenBuffers(1, &m_ssbo_positions);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_positions);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numDrops * sizeof(glm::vec4), m_positions.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_positions);

    // generate and bind ssbo_velocity
    glGenBuffers(1, &m_ssbo_velocities);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_velocities);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numDrops * sizeof(glm::vec4), m_velocities.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssbo_velocities);

#if IRIS_DEBUG
    // generate and bind ssbo_debug
    glGenBuffers(1, &m_ssbo_debug);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_debug);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numDrops * sizeof(glm::vec4), m_debugs.data(), GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ssbo_debug);
#endif

    // unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

glm::vec4 Rain::generateRainDropPosition(const glm::vec3& cloudPosition, float cloudRadius)
{
    float r = cloudRadius * (static_cast<float>(rand()) / RAND_MAX);
    float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.1415926f;

    float x = cloudPosition.x + r * cos(theta);
    float y = cloudPosition.y;
    float z = cloudPosition.z + r * sin(theta);

    return glm::vec4(x, y, z, 0.0f);
}

glm::vec4 Rain::generateRainDropVelocity(float minSpeed, float maxSpeed)
{
    float speed = minSpeed + static_cast<float>(rand()) / RAND_MAX * (maxSpeed - minSpeed);
    return glm::vec4(0.0f, -speed, 0.0f, 0.0f);
}

void Rain::computeRainOnGPU(float deltaTime, float seaLevel, const glm::vec3& cloudPosition, float cloudRadius, float minSpeed, float maxSpeed)
{
    if (m_vao == 0)
        return;

    // active the shader program
    glUseProgram(m_computeShader);

    // set uniform variables
    glUniform1f(glGetUniformLocation(m_computeShader, "deltaTime"), deltaTime);
    glUniform1f(glGetUniformLocation(m_computeShader, "seaLevel"), seaLevel);
    glUniform3fv(glGetUniformLocation(m_computeShader, "cloudPosition"), 1, glm::value_ptr(cloudPosition));
    glUniform1f(glGetUniformLocation(m_computeShader, "cloudRadius"), cloudRadius);
    glUniform1f(glGetUniformLocation(m_computeShader, "minSpeed"), minSpeed);
    glUniform1f(glGetUniformLocation(m_computeShader, "maxSpeed"), maxSpeed);

    // set Compute Shader
    glDispatchCompute((GLuint)m_positions.size() / 256 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // unbind the shader program
    glUseProgram(0);

#if IRIS_DEBUG
    // ---debug---
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_positions);
    glm::vec4* positions = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_velocities);
    glm::vec4* velocity = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_debug);
    glm::vec4* debug = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    for (int i = 0; i < m_positions.size(); i++) {
        printf("gpu rain drop (%d). position: (%f, %f, %f), velocity: (%f, %f, %f), beforeY: %f, afterY: %f, reset: %f, index: %d\n", i, positions[i].x, positions[i].y, positions[i].z, velocity[i].x, velocity[i].y, velocity[i].z, debug[i].x, debug[i].y, debug[i].z, (int)debug[i].w);
    } 
#endif
}

void Rain::renderRain(const Camera& camera, float deltaTime)
{
    if (m_vao == 0)
        return;
    
    // bind ssbo to binding 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_positions);

    // active the shader program
    glUseProgram(m_renderShader);

    // set view and projection matrices
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjMatrix();

    glUniformMatrix4fv(glGetUniformLocation(m_renderShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_renderShader, "projection"), 1, GL_FALSE, &projection[0][0]);

    // bind the vao and instanced render rain
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_POINTS, 0, 1, m_positions.size());
    glBindVertexArray(0);

    // unbind the shader program
    glUseProgram(0);
}
