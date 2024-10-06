
#include "ui.h"
#include "../graphics/window.h"
#include "../main/constants.h"
#include "../main/app_context.h"
#include <glm/gtc/type_ptr.hpp>

//#include "../volumerendering/vector.cuh"

CGRA350::AppContext *UI::m_app_context = nullptr;
bool UI::m_isChanging = false;

// Set up Dear ImGui context, backends/platforms & style
void UI::init(CGRA350::AppContext *app_context)
{
	// set finalproject app context
	UI::m_app_context = app_context;
	 
	// setup Dear ImGui context
	ImGui::CreateContext();

	// set up backend (renderer/platoform) bindings
	Window::getInstance().initImGuiGLFW();
	ImGui_ImplOpenGL3_Init();

	// set up ImGui style
	ImGui::StyleColorsDark();
}

void UI::render()
{
	// --- start context for new frame ---
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	// --- set up window  & location to top-left corner---
	ImGui::SetNextWindowPos(ImVec2(CGRA350Constants::DEFAULT_WINDOW_WIDTH - CGRA350Constants::DEFAULT_IMGUI_WIDTH, 0));
	ImGui::SetNextWindowSize(ImVec2(CGRA350Constants::DEFAULT_IMGUI_WIDTH, CGRA350Constants::DEFAULT_WINDOW_HEIGHT));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
	ImGui::Begin("CGRA350", nullptr, window_flags);

	// --- set up widgets ---
		
	// display fps
	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	// display number of primitives rendered
	//ImGui::Text("Ocean primitives: %i", m_app_context->m_num_ocean_primitives);
	//ImGui::Text("Seabed primitives: %i", m_app_context->m_num_seabed_primitives);
	ImGui::Separator();

	// --- render options
	ImGui::Text("Render Options:");

	// wireframe mode toggle (on/off)
	static bool wireframe_on = false;
	ImGui::Checkbox("Wireframe", &wireframe_on);
	if (wireframe_on)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Axis", &m_app_context->m_do_render_axis);
	ImGui::SameLine();
	ImGui::Checkbox("Grid", &m_app_context->m_do_render_grid);

	// render seabed mesh toggle (on/off)
	ImGui::Checkbox("Render Seabed", &(m_app_context->m_do_render_seabed));

	// render ocean mesh toggle (on/off)
	ImGui::Checkbox("Render Ocean", &(m_app_context->m_do_render_ocean));

	ImGui::Separator();

	// --- camera options
	ImGui::Text("Camera:");
	// direction
	float polar = m_app_context->m_render_camera.getPolarAngle();
	float azimuthal = m_app_context->m_render_camera.getAzimuthalAngle();
	float3 cameradir{ sin(polar) * cos(azimuthal), sin(polar) * sin(azimuthal), cos(polar) };
	ImGui::Text("Direction: (%.5f, %.5f, %.5f) ", cameradir.x, cameradir.y, cameradir.z);
	// polar angle	
	ImGui::InputFloat("Polar", &polar);
	m_app_context->m_render_camera.setPolarAngle(polar);
	// azimuthal angle	
	ImGui::InputFloat("Azimuthal", &azimuthal);
	m_app_context->m_render_camera.setAzimuthalAngle(azimuthal);
	// position
	glm::vec3 cam_pos = m_app_context->m_render_camera.getPosition();
	float cam_pos_a[3] = { cam_pos.x, cam_pos.y, cam_pos.z };
	ImGui::InputFloat3("Position", cam_pos_a);
	m_app_context->m_render_camera.setPosition(glm::vec3(
		cam_pos_a[0],
		cam_pos_a[1],
		cam_pos_a[2]
	));
	// speed
	float speed = m_app_context->m_render_camera.getSpeed();
	ImGui::SliderFloat("Speed", &speed, 0, CGRA350Constants::DEFAULT_CAMERA_SPEED);
	m_app_context->m_render_camera.setSpeed(speed);
	// fov
	//float fov = m_app_context->m_render_camera.getFOV();
	//ImGui::InputFloat("FOV", &fov);
	//m_app_context->m_render_camera.setFOV(fov);

	ImGui::Separator();

	// --- light options
	bool changed = false;
	ImGui::Text("Directional Light");
	// direction	
	float aziangle = m_app_context->m_gui_param.lighta;
	float altiangle = m_app_context->m_gui_param.lighty;
	float3 lightdir{ cos(aziangle) * cos(altiangle), sin(altiangle), sin(aziangle) * cos(altiangle) };
	ImGui::Text("Direction: (%.5f, %.5f, %.5f) ", lightdir.x, lightdir.y, lightdir.z);
	changed |= ImGui::SliderAngle("Azimuth", (float*)&m_app_context->m_gui_param.lighta, -180, 180);
	changed |= ImGui::SliderAngle("Altitude ", (float*)&m_app_context->m_gui_param.lighty, -90, 90);
	changed |= ImGui::ColorPicker3("Color", (float*)&m_app_context->m_gui_param.lightColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_Float | ImGuiColorEditFlags_::ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_::ImGuiColorEditFlags_HDR);
	if (changed)
		UI::SetIsChanging(true);
	ImGui::Separator();

	// --- cloud
	ImGui::Text("Volume Rendering:");
	glm::vec3 cloud_pos = m_app_context->m_gui_param.cloud_position;
	if (ImGui::InputFloat3("Cloud Position", glm::value_ptr(m_app_context->m_gui_param.cloud_position), "%.5f"))
		m_app_context->m_gui_param.frame = 0;
	ImGui::Separator();

	// --- seabed 
	// seabed mesh
	ImGui::Text("Seabed Mesh:");
	ImGui::PushItemWidth(50);
	ImGui::InputInt("Seabed Grid Width", &(m_app_context->m_seabed_grid_width), 0);
	ImGui::InputInt("Seabed Grid Length", &(m_app_context->m_seabed_grid_length), 0);
	m_app_context->m_seabed_grid_width = max(m_app_context->m_seabed_grid_width, 1);
	m_app_context->m_seabed_grid_length = max(m_app_context->m_seabed_grid_length, 1);
	ImGui::PopItemWidth();

	// seabed texture
	ImGui::Text("Seabed Texture:");
	ImGui::Combo("Tex", &(m_app_context->m_seabed_tex), "none\0sand_seabed_1\0sand_seabed_2\0pretrified_seabed");

	ImGui::Separator();

	// --- ocean 
	// ocean mesh
	ImGui::Text("Ocean Mesh:");
	ImGui::PushItemWidth(50);
	ImGui::InputInt("Ocean Width", &(m_app_context->m_ocean_width), 0);
	ImGui::InputInt("Ocean Length", &(m_app_context->m_ocean_length), 0);
	ImGui::InputInt("Ocean Grid Width", &(m_app_context->m_ocean_grid_width), 0);
	ImGui::InputInt("Ocean Grid Length", &(m_app_context->m_ocean_grid_length), 0);
	m_app_context->m_ocean_width = max(m_app_context->m_ocean_width, 1);
	m_app_context->m_ocean_length = max(m_app_context->m_ocean_length, 1);
	m_app_context->m_ocean_grid_width = max(m_app_context->m_ocean_grid_width, 1);
	m_app_context->m_ocean_grid_length = max(m_app_context->m_ocean_grid_length, 1);
	ImGui::PopItemWidth();

	// illumination model
	ImGui::Text("Illumination Model:");
	ImGui::Combo("Model", &(m_app_context->m_illumin_model), "Fresnel\0Reflection\0Refraction\0Phong\0");

	// base colour
	ImGui::Text("Water Base Colour:");
	static float water_base_colour[3] = {
		m_app_context->m_water_base_colour.r,
		m_app_context->m_water_base_colour.g,
		m_app_context->m_water_base_colour.b
	};
	ImGui::ColorEdit3("", water_base_colour);
	m_app_context->m_water_base_colour = glm::vec3(
		water_base_colour[0],
		water_base_colour[1],
		water_base_colour[2]
	);
	ImGui::SliderFloat("Amount", &(m_app_context->m_water_base_colour_amt), 0.0f, 1.0f, "%.3f");

	// --- skybox
	ImGui::Text("Environment Map:");
	ImGui::Combo("Env.Map", &(m_app_context->m_gui_param.env_map), "sky_skybox_1\0sky_skybox_2\0sunset_skybox_1\0sunset_skybox_2\0sunset_skybox_3");
		
	// --- end window
	ImGui::End();

	// --- render gui to screen ---
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::destroy()
{
	// shutdown backends
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void UI::SetIsChanging(bool isChanging)
{
	UI::m_isChanging = isChanging;
}

bool UI::GetIsChanging()
{
	return UI::m_isChanging;
}

GUIParam::GUIParam()
{
	// --- Common Settings
	float3 lightColor = { 1.0, 1.0, 1.0 };
	float3 lightDir = float3{ 0.34281, 0.70711, 0.61845 };
	this->lighta = atan2(lightDir.z, lightDir.x);
	this->lighty = atan2(lightDir.y, sqrt(max(0.0001f, lightDir.x * lightDir.x + lightDir.z * lightDir.z)));
	this->lightColor = lightColor;

	// --- Volume Rendering
	this->cloud_position = glm::vec3(-1.56288, 0.47835073, -1.2548155);
	this->G = 0.857f;
	this->alpha = 1.0f;
	this->ms = 10.0f;
	this->scatter_rate = float3{ 1, 1, 1 };

	// --- Env
	this->env_map = CGRA350Constants::DEFAULT_ENV_MAP;
}
