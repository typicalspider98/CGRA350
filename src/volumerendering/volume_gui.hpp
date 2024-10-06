#pragma once


#ifdef LINUX
#undef GUI
#endif

#ifdef GUI

#include "../graphics/camera.h"
#include "volume.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ImGui/imgui.h>

#include <iostream>
#include <chrono>
#include <thread>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <windows.h>

#include <Shlobj.h>
#pragma comment(lib,"shell32.lib")


static void init_cuda();
void CheckCuda();

class GUIs {
public:
	int width = -1;
	int height = -1;

    ImVec2 needed_size;
    int frame;
    float G;
    float alpha;
    int ms;

    bool render_surface = false;
    float IOR = 1.66;

    float tr = 1;

    float3 scatter_rate = { 1, 1, 1 };

    float env_exp = 1;

    float exposure = 1;

    float fps = 0;

    int toneType = 2;

    float lighta, lighty;
    float3 lightColor;

    bool predict = true;

    bool mrpnn = true;

    bool pause = false;

    bool denoise = true;

    bool fsr = false;

    bool checkboard = false;

    float sharpness = 0.5;

    bool change_hdri = false;

    char saveName[100] = "Noname";
    bool need_save = false;

    bool inspector = false;

    string hdri_path = "";
};

#pragma region OpenGL
static GLint add_shader(GLenum shader_type, const char* source_code, GLuint program);
static GLuint create_shader_program();

// Create a quad filling the whole screen.
static GLuint create_quad(GLuint program, GLuint* vertex_buffer);

static void resize_buffers(float3** accum_buffer_cuda, Histogram** histo_buffer_cuda, cudaGraphicsResource_t* display_buffer_cuda, GLuint tempFB, GLuint* tempTex, int width, int width2, GLuint display_buffer);
#pragma endregion

#endif

void InitCloud(Camera& cam, VolumeRender& volume, float3 lightDir = normalize({ 1,1,1 }), float3 lightColor = { 1,1,1 }, float3 scatter_rate = { 1, 1, 1 }, float alpha = 1, float multiScatterNum = 10, float g = 0);

void RenderCloud(Camera& cam, VolumeRender& volume, GLFWwindow* window, const glm::vec3& cloudPosition);

void CleanupCloud();