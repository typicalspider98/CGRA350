#ifndef APP_CONTEXT
#define APP_CONTEXT
#pragma once

#include "../graphics/camera.h"
#include "constants.h"


namespace CGRA350
{
	struct AppContext
	{
		float m_last_mouse_x;
		float m_last_mouse_y;

		int m_ocean_width = CGRA350Constants::DEFAULT_OCEAN_WIDTH;
		int m_ocean_length = CGRA350Constants::DEFAULT_OCEAN_LENGTH;
		int m_ocean_grid_width = CGRA350Constants::DEFAULT_OCEAN_GRID_WIDTH;
		int m_ocean_grid_length = CGRA350Constants::DEFAULT_OCEAN_GRID_LENGTH;

		int m_seabed_grid_width = CGRA350Constants::DEFAULT_SEABED_GRID_WIDTH;
		int m_seabed_grid_length = CGRA350Constants::DEFAULT_SEABED_GRID_LENGTH;

		bool m_do_render_cloud = true;
		bool m_do_render_seabed = false;
		bool m_do_render_ocean = false;
		bool m_do_render_ui = false;
		
		int m_illumin_model = 0; // 0: Fresnel, 1: Reflection, 2: Refraction, 3: Phong

		glm::vec3 m_water_base_colour = CGRA350Constants::DEFAULT_WATER_BASE_COLOUR;
		float m_water_base_colour_amt = CGRA350Constants::DEFAULT_WATER_BASE_COLOUR_AMOUNT;

		int m_env_map = CGRA350Constants::DEFAULT_ENV_MAP; // 0: sky_skybox_1, 1: sky_skybox_2, 3: sunset_skybox_1, 4: sunset_skybox_2, 5: sunset_skybox_3
		int m_seabed_tex = 0; // 0: none, 1: sand_seabed_1, 2: sand_seabed_2, 3: pretrified_seabed

		unsigned int m_num_ocean_primitives = 2 * CGRA350Constants::DEFAULT_OCEAN_GRID_WIDTH * CGRA350Constants::DEFAULT_OCEAN_GRID_LENGTH;
		unsigned int m_num_seabed_primitives = 2 * CGRA350Constants::DEFAULT_SEABED_GRID_WIDTH * CGRA350Constants::DEFAULT_SEABED_GRID_LENGTH;

		Camera m_render_camera;

		AppContext();
	};
}

#endif
