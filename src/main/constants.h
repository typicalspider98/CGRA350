#ifndef CGRA350_CONSTANTS
#define CGRA350_CONSTANTS
#pragma once

#include <glm/glm.hpp>
#include <string>

namespace CGRA350Constants
{
	const float DEFAULT_WINDOW_WIDTH = 1200.0f;
	const float DEFAULT_WINDOW_HEIGHT = 800.0f;
	const float DEFAULT_IMGUI_WIDTH = 400.0f;

	const unsigned int OPENGL_VERSION_MAJOR = 4;
	const unsigned int OPENGL_VERSION_MINOR = 3;

	const float CAMERA_RESOLUTION = 800.0f;
	//const float CAMERA_DISTANCE_FROM_ORIGIN = 50.0f;
	//const glm::vec3 CAMERA_POSITION = glm::vec3(
	//	CAMERA_DISTANCE_FROM_ORIGIN * glm::sin(glm::radians(45.0f)) * glm::cos(glm::radians(45.0f)), // x
	//	CAMERA_DISTANCE_FROM_ORIGIN * glm::cos(glm::radians(45.0f)), // y
	//	CAMERA_DISTANCE_FROM_ORIGIN * glm::sin(glm::radians(45.0f)) * glm::sin(glm::radians(45.0f)) // z
	//);
	const glm::vec3 CAMERA_POSITION = glm::vec3(-9.684, 24.016, 15.113);
	const float DEFAULT_CAMERA_SPEED = 30.0f;
	const float DEFAULT_CAMERA_SENSITIVITY = 0.1f;
	const float CAMERA_NEAR_PLANE = 0.1f;
	const float CAMERA_FAR_PLANE = 2000.0f;
	
	const int DEFAULT_OCEAN_WIDTH = 500;
	const int DEFAULT_OCEAN_LENGTH = 500;
	const int DEFAULT_OCEAN_GRID_WIDTH = 500;
	const int DEFAULT_OCEAN_GRID_LENGTH = 500;

	const int SEABED_EXTENSION_FROM_OCEAN = 5;
	const float SEABED_DEPTH_BELOW_OCEAN = 40.0f;
	const int DEFAULT_SEABED_GRID_WIDTH = DEFAULT_OCEAN_GRID_WIDTH / 10 + SEABED_EXTENSION_FROM_OCEAN;
	const int DEFAULT_SEABED_GRID_LENGTH = DEFAULT_OCEAN_GRID_LENGTH / 10 + SEABED_EXTENSION_FROM_OCEAN;

	const float AIR_REFRACTIVE_INDEX = 1.0003f;
	const float WATER_REFRACTIVE_INDEX = 1.3333f;

	const glm::vec3 DEFAULT_WATER_BASE_COLOUR = { 0.02f, 0.13f, 0.25f };
	const float DEFAULT_WATER_BASE_COLOUR_AMOUNT = 0.65f;

	const int DEFAULT_ENV_MAP = 0;  // 0: sky_skybox_1, 1: sky_skybox_2, 3: sunset_skybox_1, 4: sunset_skybox_2, 5: sunset_skybox_3
	const std::string ENV_FORLDER_NAME[] = { "sky_skybox_1", "sky_skybox_2", "sunset_skybox_1", "sunset_skybox_2", "sunset_skybox_3" };
	const std::string TEXTURES_FOLDER_PATH = PROJECT_SOURCE_DIR "/resources/textures/";

	// ---- Texture Sample ID
	// Lighthouse
	const int TEX_SAMPLE_ID_LIGHTHOUSE_IRON = 5;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_BLGLASS = 6;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_GLASS = 7;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_LENS = 8;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_MIRROR = 9;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_REDIRON = 10;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_ROCK = 11;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_WALL = 12;
	const int TEX_SAMPLE_ID_LIGHTHOUSE_WOOD = 13;
	
	// Tree1
	const int TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE = 15;
	const int TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE = 16;
	const int TEX_SAMPLE_ID_TREE1_LEAF_NORMAL = 27;
	const int TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR = 28;
	// Tree2
	const int TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE = 18;
	const int TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE = 19;
	const int TEX_SAMPLE_ID_TREE2_LEAF_NORMAL = 32;
	const int TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR = 33;

	// Stones
	const int TEX_SAMPLE_ID_ROCKS = 21;
	const int TEX_SAMPLE_ID_CAVEROCK = 23;
	const int TEX_SAMPLE_ID_STONE = 1;
	const int TEX_SAMPLE_ID_STONE2 = 25;

	// Cloud and Rain
	const int TEX_SAMPLE_ID_CLOUD = 30;
	const int TEX_SAMPLE_ID_RAIN_SPLASH = 31;

	// Postprocessing
	const int TEX_SAMPLE_ID_POSTPROCESSING = 20;
}

#endif