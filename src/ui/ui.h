#ifndef CGRA350_UI
#define CGRA350_UI
#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <cuda_runtime.h>
#include <vector_types.h>
#include <string>
#include <glm/glm.hpp>

namespace CGRA350 {
    class AppContext;
}

class UI
{
private:
	static CGRA350::AppContext *m_app_context;
    static bool m_isChanging;

public:
	static void init(CGRA350::AppContext *app_context);
	static void render();
	static void destroy();

    static void SetIsChanging(bool isChanging);
    static bool GetIsChanging();
};

class GUIParam {
public:
    GUIParam();

public:
    int width = -1;
    int height = -1;

    ImVec2 needed_size;
    int frame;

    glm::vec3 cloud_position;
    float G;
    float alpha;
    int ms;

    bool render_surface = false;
    float IOR = 1.66;

    float tr = 1;

    float3 scatter_rate = { 1, 1, 1 };

    int env_map = 0;
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

    std::string hdri_path = "";
};
#endif