# Team 9 - Dynamic 3D Environment Simulation

## Table of Contents
- [Project Overview](#project-overview)
- [Technologies Used](#technologies-used)
- [Project Structure](#project-structure)
- [Installation and Setup](#installation-and-setup)
- [Features](#features)
  - [Lighting and Material Modeling](#lighting-and-material-modeling)
  - [Real-Time Weather System](#real-time-weather-system)
  - [Real-Time Water and Seabed Simulation](#real-time-water-and-seabed-simulation)
- [Individual Contributions](#individual-contributions)
- [Integration Plan](#integration-plan)
- [Demo Scenario](#demo-scenario)
- [References](#references)

## Project Overview
This project aims to create a dynamic 3D environment with realistic lighting, weather effects, and water simulation. The key tasks are divided into three main sections:
1. **Lighting and material modeling** for environmental objects
2. **Real-time weather system** implementation
3. **Real-time water surface and seabed simulation**

## Technologies Used
- **C++**: Core programming language.
- **OpenGL**: Used for real-time rendering.
- **GLFW**: Window management.
- **GLM**: Mathematical operations (vector/matrix).
- **GLAD**: OpenGL function loading.
- **ImGui**: User interface.
- **CUDA**: GPU-based computation for volumetric rendering.
- **stb**: Image loading.

## Project Structure
- `src/`: Source code files for the 3D environment, weather system, water simulation, and more.
- `include/`: Header files for reusable components like lighting, materials, and rendering techniques.
- `shaders/`: GLSL shader files for lighting, water effects, and weather system rendering.
- `assets/`: Textures, models (lighthouse, rocks, plants), and other resources used in the scene.

## Features

### Lighting and Material Modeling
This feature includes:
- Realistic lighting effects on both water surfaces and environmental objects.
- Dynamic lighting adjustments based on time of day.
- Material transitions that adapt lighting effects on objects like the lighthouse and rocks.

### Real-Time Weather System
Implemented features include:
- Volumetric rendering for cloud simulation.
- Particle system to simulate rain.
- Real-time dynamic shaders for wet ground after rain.
- Weather-based scene adjustments, including color grading and light scattering.

### Real-Time Water and Seabed Simulation
This system incorporates:
- **Gerstner Waves** for real-time water surface dynamics.
- **Perlin Noise** for random ripples and natural surface variations.
- **Normal Mapping** for enhancing water surface details.
- Procedural generation of the seabed with **Perlin Noise** for realistic sand textures and variations.

## Individual Contributions

- **Hao Peng**: Lighting and material modeling for environmental objects. Focus on realistic light interactions with water surfaces and object modeling.
- **Jun Zheng**: Real-time weather system implementation, including volumetric rendering for clouds and particle systems for precipitation.
- **Ye Li**: Real-time water surface and seabed simulation, including dynamic water waves using Gerstner Waves and enhanced detail with Perlin Noise.

## Integration Plan
1. **Environment and Water Setup (19th August - 15th September)**:  
   Hao Peng focuses on environmental object modeling, Ye Li on water surface waves, and Jun Zheng on cloud simulation.
   
2. **Lighting, Materials, and Weather System (26th August - 22nd September)**:  
   Integration of lighting, material transitions, and weather system dynamics.
   
3. **Real-Time Rendering and Debugging (23rd September - 1st October)**:  
   Optimizations, color grading, and dynamic scene adjustments.
   
4. **Final Testing and Presentation Preparation (2nd October - 9th October)**:  
   System integration and demo preparation.

## Demo Scenario
In the final demo, we showcase:
- Real-time environmental interactions including lighting changes based on time of day.
- Dynamic weather effects such as rain, clouds, and ground wetness.
- Seamless transitions between beach and water environments with detailed water and seabed simulations.
- An interactive scene with user controls for toggling between weather effects, water simulations, and material adjustments.

## References
1. Bieron, J., Tong, X., Peers, P. "Single Image Neural Material Relighting." ACM SIGGRAPH 2023.
2. Aittala, M., Aila, T., and Lehtinen, J. "Reflectance Modeling by Neural Texture Synthesis." ACM TOG, 2016.
3. Van De Ruit, M., and Eisemann, E. "Metameric: Spectral Uplifting via Controllable Color Constraints." ACM SIGGRAPH 2023.
4. Hu J., Yu C., Liu H., et al. "Deep real-time volumetric rendering using multi-feature fusion." ACM SIGGRAPH 2023.
5. Jeschke, S., Pajarola, R., and Dachsbacher, C. "Water Surface Wavelets." ACM SIGGRAPH 2018.

