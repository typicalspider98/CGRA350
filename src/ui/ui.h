#ifndef CGRA350_UI
#define CGRA350_UI
#pragma once

#include "../main/app_context.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>


class UI
{
private:
	static CGRA350::AppContext *m_app_context;

public:
	static void init(CGRA350::AppContext *app_context);
	static void render();
	static void destroy();
};

#endif