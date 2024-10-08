#pragma once


#ifdef LINUX
#undef GUI
#endif

#ifdef GUI

#include "../graphics/camera.h"
#include "../ui/ui.h"
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

#pragma region OpenGL
static GLint add_shader(GLenum shader_type, const char* source_code, GLuint program);
static GLuint create_shader_program();

// Create a quad filling the whole screen.
static GLuint create_quad(GLuint program, GLuint* vertex_buffer);

static void resize_buffers(float3** accum_buffer_cuda, Histogram** histo_buffer_cuda, cudaGraphicsResource_t* display_buffer_cuda, GLuint tempFB, GLuint* tempTex, int width, int width2, GLuint display_buffer);
#pragma endregion

#endif

void InitCloud(Camera& cam, VolumeRender& volume, GUIParam& param);

void RenderCloud(Camera& cam, VolumeRender& volume, GLFWwindow* window);

void ChangeCloudEnv(VolumeRender& volume, GUIParam& param);

void CleanupCloud();