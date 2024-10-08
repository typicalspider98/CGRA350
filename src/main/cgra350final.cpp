// Local Headers
#include "cgra350final.h"
#include "constants.h"
#include "../graphics/shaders.h"
#include "../graphics/renderers.h"
#include "../graphics/textures.h"
#include "../volumerendering/vector.cuh"
#include "../computeinstancing/Rain.hpp"

#include <cuda_runtime.h>
#include <vector_types.h>

// System Headers
#include <glm/glm.hpp>
#include <vector>

int main()
{
    CGRA350::CGRA350App::initGLFW();
    CGRA350::CGRA350App finalProject; // load glfw & create window
    finalProject.initVolumeRendering();
    finalProject.renderLoop();   // render geom to window

    return EXIT_SUCCESS;
}

namespace CGRA350
{
    CGRA350App::CGRA350App() : m_window(Window::getInstance()), m_context()
    {
        // set window user pointer to the app context
        m_window.setWindowUserPointer(&m_context);

        // set up callbacks
        m_window.setCallbacks();

        // init GUI
        UI::init(&m_context);
    }

    CGRA350App::~CGRA350App()
    {
        // note: will call Window destructor & correctly destroy the window

        // cleanup UI resources
        UI::destroy();

        // terminate GLFW
        glfwTerminate();
    }

    // Init GLFW. MUST be called BEFORE creating a CGRA350App instance.
    void CGRA350App::initGLFW()
    {
        glfwInit();
    }

    void CGRA350App::initVolumeRendering()
    {
        if (m_context.m_do_render_cloud)
        {
            CheckCuda();

            string cloud_path = "./../data/CLOUD0";
            m_volumerender = new VolumeRender(cloud_path);
            InitCloud(m_context.m_render_camera, *m_volumerender, m_context.m_gui_param);
        }
    }

    void CGRA350App::renderLoop()
    {
        // ------------------------------
        // Skybox

        // Create skybox shaders
        std::vector<Shader> skybox_shaders;
        skybox_shaders.emplace_back("skybox.vert");
        skybox_shaders.emplace_back("skybox.frag");
        ShaderProgram skybox_shader_prog(skybox_shaders);

        // Create skybox cubemap texture
        std::shared_ptr<CubeMapTexture> env_maps[] = {nullptr, nullptr, nullptr, nullptr, nullptr };
        for (int i = 0; i < 5; i++)
        {
            string folder_name = CGRA350Constants::ENV_FORLDER_NAME[i];
            string env_map_imgs[] = {
                folder_name + "/right.jpg",
                folder_name + "/left.jpg",
                folder_name + "/top.jpg",
                folder_name + "/bottom.jpg",
                folder_name + "/front.jpg",
                folder_name + "/back.jpg"
            };

            env_maps[i] = std::make_shared<CubeMapTexture>(env_map_imgs);
        }

        // Track last env map used
        int last_env_map = CGRA350Constants::DEFAULT_ENV_MAP;

        // Create Skybox renderer
        SkyBoxRenderer skybox_renderer(skybox_shader_prog, *env_maps[last_env_map]);


        // ------------------------------
        // Ocean

        // --- Create Mesh
        std::shared_ptr<GridMesh> ocean_mesh_ptr = std::make_shared<GridMesh>(CGRA350Constants::DEFAULT_OCEAN_GRID_WIDTH, CGRA350Constants::DEFAULT_OCEAN_GRID_LENGTH);
        ocean_mesh_ptr->initialise();

        // --- Fresnel
        // Create ocean surface shaders
        std::vector<Shader> ocean_shaders_fresnel;
        ocean_shaders_fresnel.emplace_back("ocean_wavesim.vert");
        ocean_shaders_fresnel.emplace_back("ocean_fresnel.frag");
        ShaderProgram ocean_shader_prog_fresnel(ocean_shaders_fresnel);
        // Create ocean renderer
        FullOceanRenderer ocean_renderer_fresnel(ocean_shader_prog_fresnel, ocean_mesh_ptr, skybox_renderer.getCubeMapTexture());

        // --- Reflection
        std::vector<Shader> ocean_shaders_refl;
        ocean_shaders_refl.emplace_back("ocean_wavesim.vert");
        ocean_shaders_refl.emplace_back("ocean_refl.frag");
        ShaderProgram ocean_shader_prog_refl(ocean_shaders_refl);
        ReflectiveOceanRenderer ocean_renderer_refl(ocean_shader_prog_refl, ocean_mesh_ptr, skybox_renderer.getCubeMapTexture());

        // --- Refraction
        std::vector<Shader> ocean_shaders_refr;
        ocean_shaders_refr.emplace_back("ocean_wavesim.vert");
        ocean_shaders_refr.emplace_back("ocean_refr.frag");
        ShaderProgram ocean_shader_prog_refr(ocean_shaders_refr);
        RefractiveOceanRenderer ocean_renderer_refr(ocean_shader_prog_refr, ocean_mesh_ptr);

        // --- Phong
        std::vector<Shader> ocean_shaders_phong;
        ocean_shaders_phong.emplace_back("ocean_wavesim.vert");
        ocean_shaders_phong.emplace_back("ocean_phong.frag");
        ShaderProgram ocean_shader_prog_phong(ocean_shaders_phong);
        OceanRenderer ocean_renderer_phong(ocean_shader_prog_phong, ocean_mesh_ptr);


        // --- Other params
        // Track last ocean size values
        int last_ocean_width = CGRA350Constants::DEFAULT_OCEAN_WIDTH;
        int last_ocean_length = CGRA350Constants::DEFAULT_OCEAN_LENGTH;
        int last_ocean_mesh_grid_width = CGRA350Constants::DEFAULT_OCEAN_GRID_WIDTH;
        int last_ocean_mesh_grid_length = CGRA350Constants::DEFAULT_OCEAN_GRID_LENGTH;

        // Track last water base colour params
        glm::vec3 last_water_base_colour = CGRA350Constants::DEFAULT_WATER_BASE_COLOUR;
        float last_water_base_colour_amt = CGRA350Constants::DEFAULT_WATER_BASE_COLOUR_AMOUNT;

        // Loading Normal Maps
        Texture2D normal_map_texture = Texture2D("./water_normal1.jpg");  //Add: Water ripples
        // Passing a normal map to the reflection renderer
        ocean_renderer_refl.setNormalMapTexture(normal_map_texture);

        // Passing a normal map to the refraction renderer
        ocean_renderer_refr.setNormalMapTexture(normal_map_texture);


        // ------------------------------
        // Seabed

        // Create seabed shaders
        std::vector<Shader> seabed_shaders;
        seabed_shaders.emplace_back("seabed.vert");
        seabed_shaders.emplace_back("seabed.frag");
        ShaderProgram seabed_shader_prog(seabed_shaders);

        // Load & create Perlin noise texture
        Texture2D perlin_tex = Texture2D("perlin_noise.jpg");

        // Load & create Seabed textures
        string seabed_imgs_names[] = { "sand_seabed_1.jpg", "sand_seabed_2.jpg", "petrified_seabed.jpg" };
        std::shared_ptr<Texture2D> seabed_textures[] = { nullptr, nullptr, nullptr };
        for (int i = 0; i < 3; i++)
        {
            seabed_textures[i] = std::make_shared<Texture2D>(seabed_imgs_names[i]);
        }


        // Create seabed renderer
        SeabedRenderer seabed_renderer(seabed_shader_prog, perlin_tex);

        // Track last SEABED size values
        int last_seabed_mesh_grid_width = CGRA350Constants::DEFAULT_SEABED_GRID_WIDTH;
        int last_seabed_mesh_grid_length = CGRA350Constants::DEFAULT_SEABED_GRID_LENGTH;

        // Track last seabed texture used
        int last_seabed_tex = 0; // none

        // ------------------------------
        // Rain
        Texture2D splash_texture = Texture2D("raindrop_splash_spritesheet.png");
        std::vector<Shader> rain_compute_shader;
        rain_compute_shader.emplace_back("rain.comp");
        ShaderProgram rain_compute_shader_prog(rain_compute_shader);
        std::vector<Shader> raindrop_shaders;
        raindrop_shaders.emplace_back("raindrop.vert");
        raindrop_shaders.emplace_back("raindrop.geom");
        raindrop_shaders.emplace_back("raindrop.frag");
        ShaderProgram raindrop_shader_prog(raindrop_shaders);
        std::vector<Shader> splash_shaders;
        splash_shaders.emplace_back("splash.vert");
        splash_shaders.emplace_back("splash.frag");
        ShaderProgram splash_shader_prog(splash_shaders);
        Rain rain(rain_compute_shader_prog, raindrop_shader_prog, splash_shader_prog, splash_texture);
        int last_rain_drop_num = m_context.m_gui_param.raindrop_num;
        rain.initializeRain(last_rain_drop_num,
                            m_context.m_gui_param.rain_position,
                            m_context.m_gui_param.rain_radius,
                            m_context.m_gui_param.raindrop_min_speed,
                            m_context.m_gui_param.raindrop_max_speed);

        // ------------------------------
        // Axis
        std::vector<Shader> axis_shaders;
        axis_shaders.emplace_back("axis.vert");
        axis_shaders.emplace_back("axis.geom");
        axis_shaders.emplace_back("axis.frag");
        ShaderProgram axis_shader_prog(axis_shaders);

        // Grid
        std::vector<Shader> grid_shaders;
        grid_shaders.emplace_back("grid.vert");
        grid_shaders.emplace_back("grid.geom");
        grid_shaders.emplace_back("grid.frag");
        ShaderProgram grid_shader_prog(grid_shaders);

        // ------------------------------
        // Rendering Loop
        while (!m_window.shouldClose())
        {
            // clear window
            m_window.clear();

            // --- process keyboard input to change camera pos ---
            if (m_window.getKeyState(GLFW_KEY_W) == GLFW_PRESS) // forwards
            {
                m_context.m_render_camera.move(CameraMovement::FORWARDS, ImGui::GetIO().DeltaTime); 
            }
            if (m_window.getKeyState(GLFW_KEY_S) == GLFW_PRESS) // backwards
            {
                m_context.m_render_camera.move(CameraMovement::BACKWARDS, ImGui::GetIO().DeltaTime);
            }
            if (m_window.getKeyState(GLFW_KEY_A) == GLFW_PRESS) // left
            {
                m_context.m_render_camera.move(CameraMovement::RIGHT, ImGui::GetIO().DeltaTime);
            }
            if (m_window.getKeyState(GLFW_KEY_D) == GLFW_PRESS) // right
            {
                m_context.m_render_camera.move(CameraMovement::LEFT, ImGui::GetIO().DeltaTime);
            }
            if (m_window.getKeyState(GLFW_KEY_SPACE) == GLFW_PRESS) // upwards
            {
                m_context.m_render_camera.move(CameraMovement::UPWARDS, ImGui::GetIO().DeltaTime);
            }
            if (m_window.getKeyState(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) // downwards
            {
                m_context.m_render_camera.move(CameraMovement::DOWNWARDS, ImGui::GetIO().DeltaTime);
            }
       
            const glm::mat proj = m_context.m_render_camera.getProjMatrix();
            const glm::mat view = m_context.m_render_camera.getViewMatrix();
            const glm::vec3 cameraRight = m_context.m_render_camera.getRightVector();
            const glm::vec3 cameraUp = m_context.m_render_camera.getUpVector();

            // --- update mesh data if changed in UI ---

            // update mesh data if the grid resolution has been changed in the UI
            if (last_ocean_mesh_grid_width != m_context.m_ocean_grid_width
                || last_ocean_mesh_grid_length != m_context.m_ocean_grid_length)
            {
                ocean_renderer_fresnel.updateOceanMeshGrid(m_context.m_ocean_grid_width, m_context.m_ocean_grid_length);
                m_context.m_num_ocean_primitives = 2 * m_context.m_ocean_grid_width * m_context.m_ocean_grid_length;
                last_ocean_mesh_grid_width = m_context.m_ocean_grid_width;
                last_ocean_mesh_grid_length = m_context.m_ocean_grid_length;
            }

            if (last_seabed_mesh_grid_width != m_context.m_seabed_grid_width
                || last_seabed_mesh_grid_length != m_context.m_seabed_grid_length)
            {
                seabed_renderer.updateSeabedMeshGrid(m_context.m_seabed_grid_width, m_context.m_seabed_grid_length);
                m_context.m_num_seabed_primitives = 2 * m_context.m_seabed_grid_width * m_context.m_seabed_grid_length;
                last_seabed_mesh_grid_width = m_context.m_seabed_grid_width;
                last_seabed_mesh_grid_length = m_context.m_seabed_grid_length;
            }

            // update ocean size info if the size has been changed in the UI
            if (last_ocean_width != m_context.m_ocean_width)
            {
                ocean_renderer_fresnel.setOceanWidth(m_context.m_ocean_width);
                ocean_renderer_refl.setOceanWidth(m_context.m_ocean_width);
                ocean_renderer_refr.setOceanWidth(m_context.m_ocean_width);
                ocean_renderer_phong.setOceanWidth(m_context.m_ocean_width);
                seabed_renderer.setSeabedWidth(m_context.m_ocean_width + CGRA350Constants::SEABED_EXTENSION_FROM_OCEAN);
                last_ocean_width = m_context.m_ocean_width;
            }
            if (last_ocean_length != m_context.m_ocean_length)
            {
                ocean_renderer_fresnel.setOceanLength(m_context.m_ocean_length);
                ocean_renderer_refl.setOceanLength(m_context.m_ocean_length);
                ocean_renderer_refr.setOceanLength(m_context.m_ocean_length);
                ocean_renderer_phong.setOceanLength(m_context.m_ocean_length);
                seabed_renderer.setSeabedLength(m_context.m_ocean_length + CGRA350Constants::SEABED_EXTENSION_FROM_OCEAN);
                last_ocean_length = m_context.m_ocean_length;
            }


            // --- update water base colour params if changed in UI ---
            if (last_water_base_colour != m_context.m_water_base_colour)
            {
                ocean_renderer_fresnel.setWaterBaseColour(m_context.m_water_base_colour);
                ocean_renderer_refl.setWaterBaseColour(m_context.m_water_base_colour);
                ocean_renderer_refr.setWaterBaseColour(m_context.m_water_base_colour);
                ocean_renderer_phong.setWaterBaseColour(m_context.m_water_base_colour);
                last_water_base_colour = m_context.m_water_base_colour;
            }

            if (last_water_base_colour_amt != m_context.m_water_base_colour_amt)
            {
                ocean_renderer_fresnel.setWaterBaseColourAmount(m_context.m_water_base_colour_amt);
                ocean_renderer_refl.setWaterBaseColourAmount(m_context.m_water_base_colour_amt);
                ocean_renderer_refr.setWaterBaseColourAmount(m_context.m_water_base_colour_amt);
                last_water_base_colour_amt = m_context.m_water_base_colour_amt;
            }


            // --- update env map used if changed in UI
            if (last_env_map != m_context.m_gui_param.env_map)
            {
                skybox_renderer.setCubeMapTexture(*env_maps[m_context.m_gui_param.env_map]);
                ocean_renderer_fresnel.setSkyboxTexture(*env_maps[m_context.m_gui_param.env_map]);
                ocean_renderer_refl.setSkyboxTexture(*env_maps[m_context.m_gui_param.env_map]);
                last_env_map = m_context.m_gui_param.env_map;

                if (m_context.m_do_render_cloud)
                    ChangeCloudEnv(*m_volumerender, m_context.m_gui_param);
            }

            // --- update seabed texture used if changed in UI
            if (last_seabed_tex != m_context.m_seabed_tex)
            {
                if (m_context.m_seabed_tex == 0)
                {
                    seabed_renderer.removeSeabedTexture();
                }
                else
                {
                    seabed_renderer.setSeabedTexture(*seabed_textures[m_context.m_seabed_tex-1]);
                }
                last_seabed_tex = m_context.m_seabed_tex;
            }
            

            // --- render to texture S for refraction ---
            if (m_context.m_illumin_model % 2 == 0)
            {
                if (m_context.m_illumin_model == 0)
                {
                    ocean_renderer_fresnel.bindFBO();
                } else
                {
                    ocean_renderer_refr.bindFBO();
                }

                m_window.clear();

                // render skybox
                if (m_context.m_do_render_skybox)
                {
                    skybox_renderer.render(m_context.m_render_camera);
                }

                // render seabed
                if (m_context.m_do_render_seabed)
                {
                    seabed_renderer.render(m_context.m_render_camera);
                }

                // go back to default fbo
                if (m_context.m_illumin_model == 0)
                {
                    ocean_renderer_fresnel.unbindFBO();
                }
                else
                {
                    ocean_renderer_refr.unbindFBO();
                }
            }

            // --- render skybox ---
            if (m_context.m_do_render_skybox)
            {
                skybox_renderer.render(m_context.m_render_camera);
            }

            // --- render ocean ---
            if (m_context.m_do_render_ocean)
            {
                switch (m_context.m_illumin_model)
                {
                case 0:
                    ocean_renderer_fresnel.render(m_context.m_render_camera);
                    break;
                case 1:
                    ocean_renderer_refl.render(m_context.m_render_camera);
                    break;
                case 2:
                    ocean_renderer_refr.render(m_context.m_render_camera);
                    break;
                default:
                    ocean_renderer_phong.render(m_context.m_render_camera);
                    break;
                }
            }

            // --- render seabed ---
            if (m_context.m_do_render_seabed)
            {
                seabed_renderer.render(m_context.m_render_camera);
            }

            // --- render cloud ---
            if (m_context.m_do_render_cloud)
            {
                RenderCloud(m_context.m_render_camera, *m_volumerender, m_window.getWindow(), m_context.m_gui_param.cloud_position);
            }

            // --- render Rain ---
            if (m_context.m_do_render_rain)
            {
                if (last_rain_drop_num != m_context.m_gui_param.raindrop_num)
                {
                    last_rain_drop_num = m_context.m_gui_param.raindrop_num;
                    rain.clearRain();
                    rain.initializeRain(last_rain_drop_num, 
                                        m_context.m_gui_param.rain_position,
                                        m_context.m_gui_param.rain_radius,
                                        m_context.m_gui_param.raindrop_min_speed,
                                        m_context.m_gui_param.raindrop_max_speed);
                }

                rain.computeRainOnGPU(ImGui::GetIO().DeltaTime, 
                                m_context.m_gui_param.rain_sea_level,
                                m_context.m_gui_param.rain_position,
                                m_context.m_gui_param.rain_radius,
                                m_context.m_gui_param.raindrop_min_speed,
                                m_context.m_gui_param.raindrop_max_speed);
                rain.renderRaindrops(proj, view, ImGui::GetIO().DeltaTime,
                                m_context.m_gui_param.raindrop_length,
                                m_context.m_gui_param.raindrop_color);
                rain.renderSplashes(proj, view, cameraRight, cameraUp, ImGui::GetIO().DeltaTime);
            }
            // --- render Axis ---
            if (m_context.m_do_render_axis)
            {
                axis_shader_prog.use();
                axis_shader_prog.setMat4("uProjectionMatrix", proj);
                axis_shader_prog.setMat4("uModelViewMatrix", view);
                Renderer::draw_dummy(6);
            }

            // --- render Grid ---
            if (m_context.m_do_render_grid)
            {
                const glm::mat4 rot = glm::rotate(glm::mat4(1), glm::pi<float>() / 2.f, glm::vec3(0, 1, 0));

                grid_shader_prog.use();
                grid_shader_prog.setMat4("uProjectionMatrix", proj);
                grid_shader_prog.setMat4("uModelViewMatrix", view);
                Renderer::draw_dummy(1001);
                grid_shader_prog.setMat4("uModelViewMatrix", view * rot);
                Renderer::draw_dummy(1001);
            }

            // --- render UI ---
            if (m_context.m_do_render_ui)
            {
                UI::render();
            }

            // --- draw on screen ---
            // flip front & back buffers; and draw
            m_window.update();

            glfwPollEvents();
        }

        if (m_context.m_do_render_cloud)
        {
            CleanupCloud();
        }
    }
}
