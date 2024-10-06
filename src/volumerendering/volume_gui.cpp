#include "volume_gui.hpp"
#include "vector.cuh"
#include "../graphics/shaders.h"
#include "../main/constants.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace std;

HWND GL_Window;
HWND DX_Window;
int actual_frame = 0;
GUIs* gui = NULL;
float4* accum_buffer = NULL;
Histogram* histo_buffer_cuda = NULL;
cudaGraphicsResource_t display_buffer_cuda = NULL;
ShaderProgram* fsr_shader_prog = NULL;
GLuint tempBuffer = 0;
GLuint tempTex = 0;
GLuint display_buffer = 0;
GLuint display_tex = 0;
GLuint program = 0;
GLuint quad_vertex_buffer = 0;
GLuint quad_vao = 0;

#define CheckError { auto error = cudaGetLastError(); if (error != 0) cout << cudaGetErrorString(error) << endl; }

#pragma region Cuda
static void init_cuda()
{
    int cuda_devices[1];
    unsigned int num_cuda_devices;
    cudaGLGetDevices(&num_cuda_devices, cuda_devices, 1, cudaGLDeviceListAll);
    if (num_cuda_devices == 0) {
        fprintf(stderr, "Could not determine CUDA device for current OpenGL context\n.");
        exit(EXIT_FAILURE);
    }
    cudaSetDevice(cuda_devices[0]);
}

void CheckCuda()
{
    cudaError_t err;
    int deviceCount = 0;
    err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        std::cerr << "CUDA Error: No CUDA devices found: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    std::cout << "Number of CUDA devices: " << deviceCount << std::endl;

    int device;
    err = cudaGetDevice(&device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to get current device: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    cudaDeviceProp deviceProp;
    err = cudaGetDeviceProperties(&deviceProp, device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to get device properties: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    std::cout << "Using device: " << deviceProp.name << std::endl;

    err = cudaSetDevice(device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to set device: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    size_t freeMem, totalMem;
    cudaMemGetInfo(&freeMem, &totalMem);
    std::cout << "Free memory: " << freeMem << " bytes" << std::endl;
    std::cout << "Total memory: " << totalMem << " bytes" << std::endl;
    CheckError
        cudaFree(0);
    CheckError;
}
#pragma endregion

#pragma region Buffer
// Create a quad filling the whole screen.
static GLuint create_quad(GLuint program, GLuint* vertex_buffer)
{
    static const float3 vertices[6] = {
        { -1.f, -1.f, 0.0f },
        {  1.f, -1.f, 0.0f },
        { -1.f,  1.f, 0.0f },
        {  1.f, -1.f, 0.0f },
        {  1.f,  1.f, 0.0f },
        { -1.f,  1.f, 0.0f }
    };

    glGenBuffers(1, vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    const GLint pos_index = glGetAttribLocation(program, "Position");
    glEnableVertexAttribArray(pos_index);
    glVertexAttribPointer(
        pos_index, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

    return vertex_array;
}

static void resize_buffers(float4** accum_buffer_cuda, Histogram** histo_buffer_cuda, cudaGraphicsResource_t* display_buffer_cuda, GLuint tempFB, GLuint* tempTex, int width, int width2, GLuint display_buffer)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display_buffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * width * 4, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    if (*display_buffer_cuda)
        cudaGraphicsUnregisterResource(*display_buffer_cuda);
    cudaGraphicsGLRegisterBuffer(
        display_buffer_cuda, display_buffer, cudaGraphicsRegisterFlagsWriteDiscard);

    if (*accum_buffer_cuda)
        cudaFree(*accum_buffer_cuda);
    cudaMalloc(accum_buffer_cuda, width * width * sizeof(float4));

    if (*histo_buffer_cuda)
        cudaFree(*histo_buffer_cuda);
    cudaMalloc(histo_buffer_cuda, width * width * sizeof(Histogram));

    glDeleteTextures(1, tempTex);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFB);
    glGenTextures(1, tempTex);
    glBindTexture(GL_TEXTURE_2D, *tempTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width2, width2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *tempTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
#pragma endregion

#pragma region Render
void InitCloud(Camera& cam, VolumeRender& volume, float3 lightDir, float3 lightColor, float3 scatter_rate, float alpha, float multiScatterNum, float g) {
#ifdef GUI
    gui = new GUIs();

    gui->G = g;
    gui->alpha = alpha;
    gui->ms = multiScatterNum;
    gui->lighta = atan2(lightDir.z, lightDir.x);
    gui->lighty = atan2(lightDir.y, sqrt(max(0.0001f, lightDir.x * lightDir.x + lightDir.z * lightDir.z)));
    gui->lightColor = lightColor;
    gui->scatter_rate = scatter_rate;

    volume.UpdateHGLut(g);
    volume.SetHDRI(CGRA350Constants::TEXTURES_FOLDER_PATH + "sky_skybox_1/bottom.hdr");

    glGenBuffers(1, &display_buffer);
    glGenTextures(1, &display_tex);

    glGenFramebuffers(1, &tempBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, tempBuffer);
    glGenTextures(1, &tempTex);

    glBindTexture(GL_TEXTURE_2D, tempTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cam.GetResolution(), cam.GetResolution(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create fsr shaders
    std::vector<Shader> fsr_shaders;
    fsr_shaders.emplace_back("volumerender.vert");
    fsr_shaders.emplace_back("volumerender.frag");
    fsr_shader_prog = new ShaderProgram(fsr_shaders);

    //program = create_shader_program();
    program = fsr_shader_prog->getHandle();
    quad_vao = create_quad(program, &quad_vertex_buffer);

    init_cuda();
#endif
}

void RenderCloud(Camera& cam, VolumeRender& volume, GLFWwindow* window, const glm::vec3& cloudPosition)
{
    if (gui == NULL)
        return;

    // Save current OpenGL state
    GLboolean isCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean isDepthWriteEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &isDepthWriteEnabled);
    GLboolean isBlendEnabled = glIsEnabled(GL_BLEND);

    // Setup OpenGL state
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);

    // Reallocate buffers if window size changed.
    int nwidth, nheight;
    glfwGetFramebufferSize(window, &nwidth, &nheight);
    if (gui->fsr) {
        nwidth /= 2; nheight /= 2;
    }
    if (nwidth != gui->width || nheight != gui->height)
    {
        gui->width = nwidth;
        gui->height = nheight;

        resize_buffers(
            &accum_buffer, &histo_buffer_cuda, &display_buffer_cuda, tempBuffer, &tempTex, gui->width, gui->fsr ? gui->width * 2 : gui->width, display_buffer);
        //kernel_params.accum_buffer = accum_buffer;

        if (gui->fsr)
            glViewport(0, 0, gui->width * 2, gui->height * 2);
        else
            glViewport(0, 0, gui->width, gui->height);

        gui->frame = 0;

        //kernel_params.resolution.x = width;
        //kernel_params.resolution.y = height;
        //kernel_params.iteration = 0;

        // Allocate texture once
        glBindTexture(GL_TEXTURE_2D, display_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gui->width, gui->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Map GL buffer for access with CUDA.
    cudaGraphicsMapResources(1, &display_buffer_cuda, /*stream=*/0);
    void* p;
    size_t size_p;
    cudaGraphicsResourceGetMappedPointer(&p, &size_p, display_buffer_cuda);

    if (!gui->pause) {
        volume.SetEnvExp(gui->env_exp);
        volume.SetTrScale(gui->tr);
        volume.SetScatterRate(gui->scatter_rate * 1.001);
        volume.SetExposure(gui->exposure);
        volume.SetSurfaceIOR(gui->render_surface ? gui->IOR : -1);
        volume.SetCheckboard(gui->checkboard);

        if (gui->change_hdri) {
            volume.SetHDRI(gui->hdri_path);
            gui->change_hdri = false;
            gui->frame = 0;
        }

        if (cam.GetIsMoving())
        {
            gui->frame = 0;
            cam.SetIsMoving(false);
        }

        float3 lightDir;
        float altiangle = gui->lighty;
        lightDir.y = sin(altiangle);
        float aziangle = gui->lighta;
        lightDir.x = cos(aziangle) * cos(altiangle);
        lightDir.z = sin(aziangle) * cos(altiangle);

        float3 cameraPosition = glmVec3ToFloat3(cam.getPosition() - cloudPosition);
        float3 cameraForward = glmVec3ToFloat3(cam.getFrontVector());
        float3 cameraRight = glmVec3ToFloat3(cam.getRightVector());
        float3 cameraUp = glmVec3ToFloat3(cam.getUpVector());

        auto start_time = std::chrono::system_clock::now();
        
        volume.Render(accum_buffer, histo_buffer_cuda, reinterpret_cast<unsigned int*>(p), int2{ gui->width , gui->height },
            //cameraPosition, cameraUp, cameraRight,
            cameraPosition, cameraForward, cameraUp, cameraRight,
            lightDir, gui->lightColor, gui->alpha, gui->ms, gui->G, gui->frame,
            gui->predict ? (gui->mrpnn ? VolumeRender::RenderType::MRPNN : VolumeRender::RenderType::RPNN) : VolumeRender::RenderType::PT,
            gui->toneType, gui->denoise);

        auto finish_time = std::chrono::system_clock::now();
        float new_fps = 10000000.0f / (finish_time - start_time).count();
        gui->fps = lerp(gui->fps, new_fps, abs(new_fps - gui->fps) / gui->fps > 0.3 ? 1.0f : 0.01f);

        gui->frame++;
        actual_frame++;
    }

#ifdef IRIS_DEBUG
    size_t bufferSize = gui->width * gui->width * sizeof(float4);
    float4* host_buffer = (float4*)malloc(bufferSize);
    cudaMemcpy(host_buffer, accum_buffer, bufferSize, cudaMemcpyDeviceToHost);
    for (int i = 260; i < 300; i++) {
        int j = 520;
        int idx = i * gui->width + j;
        printf("CUDA Pixel (%d, %d): R=%f, G=%f, B=%f, A=%f\n", i, j, host_buffer[idx].x, host_buffer[idx].y, host_buffer[idx].z, host_buffer[idx].w);
    }
#endif

    // Unmap GL buffer.
    cudaGraphicsUnmapResources(1, &display_buffer_cuda, /*stream=*/0);

    // Update texture for display.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display_buffer);
    glBindTexture(GL_TEXTURE_2D, display_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gui->width, gui->height, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

#ifdef IRIS_DEBUG
    GLubyte* textureData = new GLubyte[gui->width * gui->height * 4];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
    for (int i = 260; i < 300; i++) {
        int j = 520;
        int idx = i * gui->width + j;
        printf("Tex Pixel (%d, %d): R=%d, G=%d, B=%d, A=%d\n", i, j, textureData[idx * 4], textureData[idx * 4 + 1], textureData[idx * 4 + 2], textureData[idx * 4 + 3]);
    }
    delete[] textureData;
#endif

    // Render the quad.
    glUseProgram(program);

    // Caculate and apply the MVP matrix
    glm::mat4 model = glm::translate(glm::mat4(1.0f), cloudPosition);
    glm::mat4 view = cam.getViewMatrix();
    //glm::vec3 up(0.0f, 1.0f, 0.0f);
    //glm::vec3 camPos = cam.getPosition() / glm::vec3(200, 200, 200);
    //glm::vec3 target = camPos + cam.getFrontVector();
    //glm::mat4 view = glm::lookAt(camPos, target, up);
    glm::mat4 projection = cam.getProjMatrix();
    glm::mat4 MVP = projection* view* model;
    glUniformMatrix4fv(glGetUniformLocation(program, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));

    //glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quad_vao);

    glUniform1i(glGetUniformLocation(program, "Size"), gui->width);
    if (gui->fsr) {
        glUniform1f(glGetUniformLocation(program, "sharp"), gui->sharpness);
        glBindFramebuffer(GL_FRAMEBUFFER, tempBuffer);
        glUniform1i(glGetUniformLocation(program, "FSR"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, tempTex);
        glUniform1i(glGetUniformLocation(program, "FSR"), 2);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUniform1i(glGetUniformLocation(program, "FSR"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Reset OpenGL state
    if (isCullFaceEnabled) glEnable(GL_CULL_FACE);
    if (isDepthWriteEnabled) glDepthMask(true);
    if (!isBlendEnabled) glDisable(GL_BLEND);

}

void CleanupCloud()
{
    cudaFree(accum_buffer);

    // Cleanup OpenGL.
    glDeleteVertexArrays(1, &quad_vao);
    glDeleteBuffers(1, &quad_vertex_buffer);
    //glDeleteProgram(program);
    if (fsr_shader_prog != NULL)
    {
        delete fsr_shader_prog;
        fsr_shader_prog = NULL;
    }
}
#pragma endregion