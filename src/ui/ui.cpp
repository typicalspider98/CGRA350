
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
	//ImGui::SetNextWindowPos(ImVec2(CGRA350Constants::DEFAULT_WINDOW_WIDTH - CGRA350Constants::DEFAULT_IMGUI_WIDTH, 0));
	//ImGui::SetNextWindowSize(ImVec2(CGRA350Constants::DEFAULT_IMGUI_WIDTH, CGRA350Constants::DEFAULT_WINDOW_HEIGHT));
	
	// This is only called during initial setup.
	ImGui::SetNextWindowPos(ImVec2(CGRA350Constants::DEFAULT_WINDOW_WIDTH - CGRA350Constants::DEFAULT_IMGUI_WIDTH, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(CGRA350Constants::DEFAULT_IMGUI_WIDTH, CGRA350Constants::DEFAULT_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);

	//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
	//ImGui::Begin("CGRA350", nullptr, window_flags);
	ImGui::Begin("CGRA350");

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
	ImGui::SameLine();
	// render ocean mesh toggle (on/off)
	ImGui::Checkbox("Render Ocean", &(m_app_context->m_do_render_ocean));
	ImGui::Checkbox("Render Cloud", &(m_app_context->m_do_render_cloud));
	ImGui::Checkbox("Postprocessing", &(m_app_context->m_do_render_postprocessing));
	ImGui::Separator();

	// --- camera options
	ImGui::Text("Camera:");
	// direction
	float polar = m_app_context->m_render_camera.getPolarAngle();
	float azimuthal = m_app_context->m_render_camera.getAzimuthalAngle();
	float3 cameradir{ sin(polar) * cos(azimuthal), sin(polar) * sin(azimuthal), cos(polar) };
	ImGui::Text("Direction: (%.5f, %.5f, %.5f) ", cameradir.x, cameradir.y, cameradir.z);
	// polar angle	
	if (ImGui::InputFloat("Polar", &polar))
		m_app_context->m_render_camera.setPolarAngle(polar);
	// azimuthal angle	
	if (ImGui::InputFloat("Azimuthal", &azimuthal))
		m_app_context->m_render_camera.setAzimuthalAngle(azimuthal);
	// position
	glm::vec3 cam_pos = m_app_context->m_render_camera.getPosition();
	float cam_pos_a[3] = { cam_pos.x, cam_pos.y, cam_pos.z };
	if (ImGui::InputFloat3("Position", cam_pos_a))
		m_app_context->m_render_camera.setPosition(glm::vec3(
			cam_pos_a[0],
			cam_pos_a[1],
			cam_pos_a[2]
		));
	// speed
	float speed = m_app_context->m_render_camera.getSpeed();
	if (ImGui::SliderFloat("Speed", &speed, 0, CGRA350Constants::DEFAULT_CAMERA_SPEED))
		m_app_context->m_render_camera.setSpeed(speed);
	// fov
	//float fov = m_app_context->m_render_camera.getFOV();
	//ImGui::InputFloat("FOV", &fov);
	//m_app_context->m_render_camera.setFOV(fov);

	// Default Pos
	ImGui::Text("Look at:");
	if (ImGui::Button("Cloud"))
	{
		glm::vec3 camera_pos = glm::vec3(0.301, 0.068, 0.971) + m_app_context->m_gui_param.cloud_position;
		m_app_context->m_render_camera.setPosition(camera_pos);
		m_app_context->m_render_camera.setPolarAngle(-4);
		m_app_context->m_render_camera.setAzimuthalAngle(-90);
	}
	ImGui::SameLine();
	if (ImGui::Button("Rain"))
	{
		glm::vec3 camera_pos = glm::vec3(-0.04156, 0.00043, -0.99914);
		m_app_context->m_render_camera.setPosition(camera_pos);
		m_app_context->m_render_camera.setPolarAngle(3.1);
		m_app_context->m_render_camera.setAzimuthalAngle(-97.4);
	}
	ImGui::Separator();

	// --- objects appear
	ImGui::Text("Objects Appear Control:");

	ImGui::Checkbox("Lighthouse", &m_app_context->m_appear_lighthouse);
	ImGui::Checkbox("Tree", &m_app_context->m_appear_tree);
	ImGui::Checkbox("Stone", &m_app_context->m_appear_stone);

	ImGui::Separator();

	// --- light options
	bool changed = false;
	ImGui::Text("Directional Light:");
	// direction
	glm::vec3 lightdir = m_app_context->m_gui_param.GetDLight_Direction();
	ImGui::Text("Direction: (%.5f, %.5f, %.5f) ", lightdir.x, lightdir.y, lightdir.z);
	changed |= ImGui::SliderAngle("Azimuth", (float*)&m_app_context->m_gui_param.dlight_azimuth, -180, 180);
	changed |= ImGui::SliderAngle("Altitude", (float*)&m_app_context->m_gui_param.dlight_altitute, -180, 180);
	changed |= ImGui::ColorEdit3("Color", (float*)&m_app_context->m_gui_param.dlight_color);

	ImGui::SliderFloat("Light Strength", &m_app_context->m_gui_param.dlight_strength, 0.0f, 20.0f);
	
	ImGui::Separator();

	// --- material options
	//bool changed = false;
	ImGui::Text("Lighthouse's Material:");
	// direction
	ImGui::Combo("Wall Material", &(m_app_context->m_wall_material), "white stone\0wood\0concrete\0grey_new_brick\0marble\0stainless_steel\0brushed_medal");
	ImGui::Combo("Roof Material", &(m_app_context->m_roof_material), "tiles\0medal\0marble\0rusty_medal\0concrete\0stainless_steel\0glavanized_medal\0wood");
	ImGui::Combo("Bottom Material", &(m_app_context->m_bottom_material), "brick\0soil\0sand\0marble\0block_stone\0cobblestone\0medal\0concrete\0wood");
	ImGui::Combo("Light Model", &(m_app_context->m_light_model), "None\0Cook-Torrance\0Oren-Nayar");

	ImGui::SliderFloat("Roughness", &m_app_context->m_lighthouse_roughness, 0.0f, 3.0f);
	ImGui::SliderFloat("Metalness", &m_app_context->m_lighthouse_medalness, 0.0f, 3.0f);
	ImGui::SliderFloat("Reflectivity", &m_app_context->m_lighthouse_reflectivity, 0.0f, 3.0f);

	ImGui::Separator();

	// --- cloud
	ImGui::Text("Volume Rendering:");
	glm::vec3 cloud_pos = m_app_context->m_gui_param.cloud_position;	
	changed |= ImGui::InputFloat3("Cloud Position", glm::value_ptr(m_app_context->m_gui_param.cloud_position), "%.5f");
	changed |= ImGui::SliderFloat("G", &m_app_context->m_gui_param.G, 0, 0.857);
	changed |= ImGui::SliderFloat("Alpha", &m_app_context->m_gui_param.alpha, 0.1, 10);
	changed |= ImGui::SliderInt("Multi Scatter", &m_app_context->m_gui_param.ms, 1, 1000);
	changed |= ImGui::SliderFloat("Tr scale", &m_app_context->m_gui_param.tr, 1, 10);
	changed |= ImGui::SliderFloat3("Scatter rate", (float*)&m_app_context->m_gui_param.scatter_rate, 0, 1);
	changed |= ImGui::SliderFloat("Scale (Experimental)", &m_app_context->m_gui_param.cloud_scale, 0.1, 15);
	if (changed)
		UI::SetIsChanging(true);
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
	ImGui::Separator();

	// --- rain
	ImGui::Text("Rain:");
	ImGui::SliderFloat("Rain Radius", &m_app_context->m_gui_param.rain_radius, 0.0f, 100.0f, "%.1f");
	ImGui::InputInt("Rain Drop Number", &m_app_context->m_gui_param.raindrop_num);
	ImGui::SliderFloat("Rain Min Speed", &m_app_context->m_gui_param.raindrop_min_speed, 0.0f, 100.0f, "%.1f");
	ImGui::SliderFloat("Rain Max Speed", &m_app_context->m_gui_param.raindrop_max_speed, 0.0f, 100.0f, "%.1f");
	ImGui::SliderFloat("Sea Level", &m_app_context->m_gui_param.rain_sea_level, -150.0f, 50.0f, "%.1f");
	ImGui::Separator();

	// --- postprocessing
	ImGui::Text("Post-processing:");
	// Color Grading
	ImGui::Checkbox("Enable Color Grading", &m_app_context->m_gui_param.enableColorGrading);
	ImGui::ColorEdit3("Color Filter", (float*)&m_app_context->m_gui_param.colorFilter);
	// Dynamic Filter
	ImGui::Checkbox("Enable Dynamic Filter", &m_app_context->m_gui_param.enableDynamicFilter);
	ImGui::SliderFloat("Contrast", &m_app_context->m_gui_param.contrast, 0.0f, 2.0f);
	ImGui::SliderFloat("Brightness", &m_app_context->m_gui_param.brightness, -1.0f, 1.0f);
	// Gaussian Blur
	ImGui::Checkbox("Enable Gaussian Blur", &m_app_context->m_gui_param.enableGaussianBlur);
	ImGui::SliderFloat2("Blur Direction", (float*)&m_app_context->m_gui_param.blurDirection, -1.0f, 1.0f);
	ImGui::SliderFloat("Blur Weight 0", &m_app_context->m_gui_param.blurWeight[0], 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Weight 1", &m_app_context->m_gui_param.blurWeight[1], 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Weight 2", &m_app_context->m_gui_param.blurWeight[2], 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Weight 3", &m_app_context->m_gui_param.blurWeight[3], 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Weight 4", &m_app_context->m_gui_param.blurWeight[4], 0.0f, 1.0f);
	// Bloom Effect
	ImGui::Checkbox("Enable Bloom", &m_app_context->m_gui_param.enableBloom);
	ImGui::SliderFloat("Bloom Threshold", &m_app_context->m_gui_param.bloomThreshold, 0.0f, 5.0f);
	ImGui::SliderFloat("Bloom Intensity", &m_app_context->m_gui_param.bloomIntensity, 0.0f, 5.0f);

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
	float screenWidth = CGRA350Constants::DEFAULT_WINDOW_WIDTH;

	// --- Directional Light
	float3 lightColor = { 1.0, 1.0, 1.0 };
	float3 lightDir = float3{ -0.829, 0.60, -0.545 };
	this->dlight_azimuth = atan2(lightDir.z, lightDir.x);
	this->dlight_altitute = atan2(lightDir.y, sqrt(max(0.0001f, lightDir.x * lightDir.x + lightDir.z * lightDir.z)));
	this->dlight_color = lightColor;
	this->dlight_strength = 1;

	//// Point Light
	//this->plight_position = glm::vec3(-15.0, 85.0, -15.0);
	//this->plight_color = glm::vec3(1, 1, 1);
	//this->plight_strength = 1;

	// --- Volume Rendering
	this->cloud_position = glm::vec3(-35, 25, -5);
	this->cloud_scale = 1;
	this->G = 0.857f;
	this->alpha = 1.0f;
	this->ms = 10.0f;
	this->scatter_rate = float3{ 1, 1, 1 };

	// --- Env
	this->env_map = CGRA350Constants::DEFAULT_ENV_MAP;

	// --- Rain
	this->rain_position = glm::vec3(-60, 100, -60);
	this->rain_radius = 86.0f;
	this->rain_sea_level = -8.8f;
	this->raindrop_num = 0;
	this->raindrop_length = 0.8f;
	this->raindrop_color = glm::vec3(0.635f, 0.863f, 0.949f);	
	this->raindrop_min_speed = 11.5f;
	this->raindrop_max_speed = 17.1f;

	// --- Postprocessing
	// Color Grading
	this->enableColorGrading = false;
	this->colorFilter = glm::vec3(0.8f, 0.8f, 1.1f);

	// Dynamic Filter
	this->enableDynamicFilter = false;
	this->contrast = 1.2f;
	this->brightness = -0.1f;

	// Gaussian Blur
	this->enableGaussianBlur = false;
	this->blurDirection = glm::vec2(1.0f, 0.0f);
	this->blurWeight[0] = 0.227027f;
	this->blurWeight[1] = 0.1945946f;
	this->blurWeight[2] = 0.1216216f;
	this->blurWeight[3] = 0.054054f;
	this->blurWeight[4] = 0.016216f;
	this->blurOffset[0] = glm::vec2(1.0f / screenWidth, 0.0f);
	this->blurOffset[1] = glm::vec2(2.0f / screenWidth, 0.0f);
	this->blurOffset[2] = glm::vec2(3.0f / screenWidth, 0.0f);
	this->blurOffset[3] = glm::vec2(4.0f / screenWidth, 0.0f);
	this->blurOffset[4] = glm::vec2(5.0f / screenWidth, 0.0f);

	// Bloom Effect
	this->enableBloom = false;
	this->bloomThreshold = 1.0f;
	this->bloomIntensity = 0.8f;
}

glm::vec3 GUIParam::GetDLight_Direction() const
{
	float aziangle = dlight_azimuth;
	float altiangle = dlight_altitute;
	glm::vec3 lightdir(cos(aziangle) * cos(altiangle), sin(altiangle), sin(aziangle) * cos(altiangle));
	return lightdir;
}
