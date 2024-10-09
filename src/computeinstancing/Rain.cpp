#include "Rain.hpp"

#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Rain::Rain(ShaderProgram& computeShader, ShaderProgram& raindropShader, ShaderProgram& splashShader, const Texture2D& splash_texture) :
    m_computeShader(computeShader), m_raindropShader(raindropShader), m_splashShader(splashShader),
    m_splash_texture(splash_texture)
{
    m_raindrop_num = 0;
    m_splash_max = 0;

    m_raindrop_vao = 0;
    m_raindrop_ssbo = 0;

    m_splash_vao = 0;
    m_splash_vbo = 0;
    m_splash_ssbo = 0;

    m_splashShader.use();
    m_splashShader.setInt("spriteTexture", CGRA350Constants::TEX_SAMPLE_ID_RAIN_SPLASH);
}

void Rain::setupShadersAndBuffers()
{
    // raindrop: vao and ssbo
    glGenVertexArrays(1, &m_raindrop_vao);
    glGenBuffers(1, &m_raindrop_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_raindrop_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Raindrop) * m_raindrop_num, m_raindrops.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_raindrop_ssbo);

    // splash: vao and vbo
    GLfloat quadVertices[] = {
        -1.0f, -0.5f,       // left-bottom
         1.0f, -0.5f,       // right-bottom
         1.0f,  0.5f,       // right-top
        -1.0f,  0.5f        // left-top
    };
    glGenVertexArrays(1, &m_splash_vao);
    glGenBuffers(1, &m_splash_vbo);
    glBindVertexArray(m_splash_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_splash_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);    

    // splash: ssbo
    glGenBuffers(1, &m_splash_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_splash_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Splash) * m_splash_max, m_splashes.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_splash_ssbo);

#if IRIS_DEBUG
    // debug: ssbo
    glGenBuffers(1, &m_debug_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_debug_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * m_raindrop_num, m_debugs.data(), GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_debug_ssbo);

    printf("Setup buffers. Size of Raindrop: %zu\n", sizeof(Raindrop));
    printf("Setup buffers. Alignment of Raindrop: %zu\n", alignof(Raindrop));

    printf("Setup buffers. Size of Splash: %zu\n", sizeof(Splash));
    printf("Setup buffers. Alignment of Splash: %zu\n", alignof(Splash));
#endif
}

void Rain::clearRain()
{
    m_raindrops.clear();
    m_splashes.clear();

    if (m_raindrop_vao) {
        glDeleteVertexArrays(1, &m_raindrop_vao);
        m_raindrop_vao = 0;
    }
    if (m_raindrop_ssbo) {
        glDeleteBuffers(1, &m_raindrop_ssbo);
        m_raindrop_ssbo = 0;
    }

    if (m_splash_vao) {
        glDeleteVertexArrays(1, &m_splash_vao);
        m_splash_vao = 0;
    }
    if (m_splash_ssbo) {
        glDeleteVertexArrays(1, &m_splash_ssbo);
        m_splash_ssbo = 0;
    }

#if IRIS_DEBUG
    m_debugs.clear();
    if (m_debug_ssbo) {
        glDeleteBuffers(1, &m_debug_ssbo);
        m_debug_ssbo = 0;
    }
#endif
}

void Rain::initializeRain(int numDrops, const glm::vec3& rainPosition, float cloudRadius, float minSpeed, float maxSpeed, float seaLevel)
{
    m_raindrop_num = numDrops;
    m_splash_max = numDrops;

    for (int i = 0; i < numDrops; i++) 
    {
        glm::vec4 position = generateRainDropPosition(rainPosition, cloudRadius, seaLevel);
        glm::vec4 velocity = generateRainDropVelocity(minSpeed, maxSpeed);
        Raindrop raindrop;
        raindrop.position = position;
        raindrop.velocity = velocity;
        m_raindrops.push_back(raindrop);

        Splash splash;
        splash.position = glm::vec4(0, 0, 0, 1);
        splash.lifetime = glm::vec4(0, -1, 0, 0);
        m_splashes.push_back(splash);

#if IRIS_DEBUG
        m_debugs.push_back(glm::vec4(0, 0, 0, 0));
        printf("cpu rain drop (%d). position: (%f, %f, %f), velocity: (%f, %f, %f)\n", i, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
#endif
    }

    setupShadersAndBuffers();
}

glm::vec4 Rain::generateRainDropPosition(const glm::vec3& rainPosition, float cloudRadius, float seaLevel)
{
    float r = cloudRadius * (static_cast<float>(rand()) / RAND_MAX);
    float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.1415926f;

    float x = rainPosition.x + r * cos(theta);
    float y = seaLevel + (static_cast<float>(rand()) / RAND_MAX) * (rainPosition.y - seaLevel);
    float z = rainPosition.z + r * sin(theta);

    return glm::vec4(x, y, z, 1.0f);
}

glm::vec4 Rain::generateRainDropVelocity(float minSpeed, float maxSpeed)
{
    float speed = minSpeed + static_cast<float>(rand()) / RAND_MAX * (maxSpeed - minSpeed);
    return glm::vec4(0.0f, -speed, 0.0f, 0.0f);
}

void Rain::computeRainOnGPU(float deltaTime, float seaLevel, const glm::vec3& rainPosition, float cloudRadius, float minSpeed, float maxSpeed)
{
    if (m_raindrop_ssbo == 0)
        return;

    // active the shader program
    m_computeShader.use();

    // set uniform variables
    //m_computeShader.setFloat("deltaTime", deltaTime);

    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "deltaTime"), deltaTime);
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "seaLevel"), seaLevel);
    glUniform3fv(glGetUniformLocation(m_computeShader.getHandle(), "rainPosition"), 1, glm::value_ptr(rainPosition));
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "cloudRadius"), cloudRadius);
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "minSpeed"), minSpeed);
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "maxSpeed"), maxSpeed);
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "minLifetime"), 3.0f);
    glUniform1f(glGetUniformLocation(m_computeShader.getHandle(), "maxLifetime"), 5.0f);
    glUniform1ui(glGetUniformLocation(m_computeShader.getHandle(), "maxSplashes"), m_splash_max);

    // set Compute Shader
    glDispatchCompute((GLuint)m_raindrops.size() / 256 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // unbind the shader program
    glUseProgram(0);

#if IRIS_DEBUG
    // ---debug---
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_raindrop_ssbo);
    Raindrop* raindrops = (Raindrop*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_splash_ssbo);
    Splash* splashes = (Splash*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_debug_ssbo);
    glm::vec4* debug = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    for (int i = 0; i < m_raindrops.size(); i++) {
        printf("gpu rain drop (%d). position: (%f, %f, %f), velocity: (%f, %f, %f), beforeY: %f, afterY: %f, reset: %f, index: %d, splash position: (%f, %f, %f), total lifetime: %f, left lifetime: %f\n", 
                i, raindrops[i].position.x, raindrops[i].position.y, raindrops[i].position.z, 
                raindrops[i].velocity.x, raindrops[i].velocity.y, raindrops[i].velocity.z,
                debug[i].x, debug[i].y, debug[i].z, (int)debug[i].w,
                splashes[i].position.x, splashes[i].position.y, splashes[i].position.z, 
                splashes[i].lifetime.x, splashes[i].lifetime.y);
    } 
#endif
}

void Rain::renderRaindrops(const glm::mat4& projection, const glm::mat4& view, float deltaTime, float raindrop_length, const glm::vec3& raindrop_color)
{
    if (m_raindrop_ssbo == 0)
        return;   

    // active the shader program
    m_raindropShader.use();
    glEnable(GL_PROGRAM_POINT_SIZE);

    // set uniform variables
    glUniformMatrix4fv(glGetUniformLocation(m_raindropShader.getHandle(), "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_raindropShader.getHandle(), "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1f(glGetUniformLocation(m_raindropShader.getHandle(), "raindrop_length"), raindrop_length);
    glUniform3fv(glGetUniformLocation(m_raindropShader.getHandle(), "raindrop_color"), 1, glm::value_ptr(raindrop_color));

    // bind ssbo to binding 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_raindrop_ssbo);

    // bind the vao and instanced render raindrops
    glBindVertexArray(m_raindrop_vao);
    glDrawArraysInstanced(GL_POINTS, 0, 1, m_raindrop_num);
    glBindVertexArray(0);
        
    // unbind the shader program
    glDisable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(0);    
}

void Rain::renderSplashes(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraRight, const glm::vec3& cameraUp, float deltaTime)
{
    if (m_splash_ssbo == 0)
        return;

    // active the shader program
    m_splashShader.use();

    // active and bind texture
    glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_RAIN_SPLASH);
    m_splash_texture.bind();

    // set uniform variables
    glUniformMatrix4fv(glGetUniformLocation(m_splashShader.getHandle(), "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_splashShader.getHandle(), "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(m_splashShader.getHandle(), "cameraRight"), 1, glm::value_ptr(cameraRight));
    glUniform3fv(glGetUniformLocation(m_splashShader.getHandle(), "cameraUp"), 1, glm::value_ptr(cameraUp));

    // bind ssbo to binding 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_splash_ssbo);

    // bind the vao and instanced render splashes
    glBindVertexArray(m_splash_vao);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, m_splash_max);
    glBindVertexArray(0);

    // unbind the shader program
    glUseProgram(0);
}
