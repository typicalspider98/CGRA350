// Local Headers
#include "cgra350final.h"
#include "constants.h"
#include "../graphics/shaders.h"
#include "../graphics/renderers.h"
#include "../graphics/textures.h"


// System Headers
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

const std::string BASE_PATH = "D:/CGRA350 Assignment/Final Project/Debug/CGRA350-main_16/CGRA350-main/resources/assets/";
const std::string BASE_PATH_TEXTURE = "D:/CGRA350 Assignment/Final Project/Debug/CGRA350-main_16/CGRA350-main/resources/textures/";
const std::string LIGHTHOUSE_TEXTURE = "D:/CGRA350 Assignment/Final Project/Debug/CGRA350-main_16/CGRA350-main/resources/textures/lighthouse/";

int main()
{
    CGRA350::CGRA350App::initGLFW();
    CGRA350::CGRA350App finalProject; // load glfw & create window
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

    // 加载obj
    struct wavefront_vertex {
        unsigned int p, n, t;
    };

    ObjMesh load_wavefront_obj(const std::string& filepath) {
        using namespace std;
        using namespace glm;

        vector<vec3> positions;
        vector<vec3> normals;
        vector<vec2> uvs;
        vector<wavefront_vertex> wv_vertices;
        vector<unsigned int> indices;

        std::map<std::string, MeshPart> meshParts;
        std::string currentMaterialName;

        ifstream objFile(filepath);
        if (!objFile.is_open()) {
            cerr << "Error: could not open " << filepath << endl;
            throw runtime_error("Error: could not open file " + filepath);
        }

        // 解析 .obj 文件
        while (objFile.good()) {
            string line;
            getline(objFile, line);
            istringstream objLine(line);
            string mode;
            objLine >> mode;

            if (mode == "v") {
                vec3 v;
                objLine >> v.x >> v.y >> v.z;
                positions.push_back(v);
            }
            else if (mode == "vn") {
                vec3 vn;
                objLine >> vn.x >> vn.y >> vn.z;
                normals.push_back(vn);
            }
            else if (mode == "vt") {
                vec2 vt;
                objLine >> vt.x >> vt.y;
                uvs.push_back(vt);
            }
            else if (mode == "f") {
                vector<wavefront_vertex> face;
                while (objLine.good()) {
                    wavefront_vertex v;
                    objLine >> v.p;

                    if (objLine.peek() == '/') {
                        objLine.ignore(1);
                        if (objLine.peek() != '/') {
                            objLine >> v.t;
                        }
                        if (objLine.peek() == '/') {
                            objLine.ignore(1);
                            objLine >> v.n;
                        }
                    }

                    v.p -= 1;
                    v.n -= 1;
                    v.t -= 1;
                    face.push_back(v);
                }

                // 处理三角形面
                if (face.size() == 3) {
                    for (int i = 0; i < 3; ++i) {
                        wv_vertices.push_back(face[i]);
                    }

                    for (int i = 0; i < 3; ++i) {
                        // 将面加入当前材质对应的部分
                        meshParts[currentMaterialName].positions.push_back(positions[face[i].p]);
                        meshParts[currentMaterialName].normals.push_back(normals[face[i].n]);
                        meshParts[currentMaterialName].texCoords.push_back(uvs[face[i].t]);
                        meshParts[currentMaterialName].indices.push_back(meshParts[currentMaterialName].positions.size() - 1);
                    }
                }
            }
            else if (mode == "usemtl") {
                // 处理新的材质
                objLine >> currentMaterialName;
            }
        }

        // 生成法线
        if (normals.empty()) {
            // Generate normals if not present
            for (size_t i = 0; i < positions.size(); ++i) {
                normals.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
            }

            // Compute normals
            for (size_t i = 0; i < indices.size(); i += 3) {
                glm::vec3 v0 = positions[indices[i]];
                glm::vec3 v1 = positions[indices[i + 1]];
                glm::vec3 v2 = positions[indices[i + 2]];

                glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                normals[indices[i]] += normal;
                normals[indices[i + 1]] += normal;
                normals[indices[i + 2]] += normal;
            }

            for (size_t i = 0; i < normals.size(); ++i) {
                normals[i] = glm::normalize(normals[i]);
            }
        }

        // 构建索引
        for (const wavefront_vertex& v : wv_vertices) {
            indices.push_back(v.p);
        }

        // 创建 ObjMesh 对象
        ObjMesh objMesh(positions, normals, uvs, indices);
        objMesh.initialise();  // 初始化 OpenGL 数据

        for (auto& part : meshParts) {
            part.second.initialise();
            objMesh.parts[part.first] = part.second;
        }

        return objMesh;
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
        string folder_names[] = { "sky_skybox_1", "sky_skybox_2", "sunset_skybox_1", "sunset_skybox_2", "sunset_skybox_3" };
        std::shared_ptr<CubeMapTexture> env_maps[] = {nullptr, nullptr, nullptr, nullptr, nullptr };
        for (int i = 0; i < 5; i++)
        {
            string folder_name = folder_names[i];
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

        // 加载法线贴图
        Texture2D normal_map_texture = Texture2D("./water_normal1.jpg");  //Add: 水面波纹
        // 将法线贴图传递给反射渲染器
        ocean_renderer_refl.setNormalMapTexture(normal_map_texture);

        // 将法线贴图传递给折射渲染器
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

        //-----------------------//
        // 加载灯塔模型
        // 加载 teapot.obj 文件
        ObjMesh lighthouseMesh = load_wavefront_obj(BASE_PATH + "lighthouse9.obj");        

        std::vector<Shader> lighthouse_shaders;
        lighthouse_shaders.emplace_back("lighthouse.vert");
        lighthouse_shaders.emplace_back("lighthouse.frag");
        ShaderProgram lighthouse_shader_prog(lighthouse_shaders);

        // 加载灯塔墙壁的法线贴图或颜色贴图
        //Texture2D lighthouse_texture = Texture2D("./lighthouse_wall4.jpg");  // 载入纹理

        Texture2D lighthouse_iron = Texture2D("./Lighthouse_Material/Floor2.jpg");  //
        Texture2D lighthouse_blglass = Texture2D("./Lighthouse_Material/window4.jpg");   //
        Texture2D lighthouse_glass = Texture2D("./Lighthouse_Material/Wooden_door.jpg");  //
        Texture2D lighthouse_lens = Texture2D("./Lighthouse_Material/Handle0.jpg");
        Texture2D lighthouse_mirror = Texture2D("./Lighthouse_Material/window2.jpg");  //
        Texture2D lighthouse_rediron = Texture2D("./Lighthouse_Material/roof2.jpg"); // roof
        Texture2D lighthouse_rock = Texture2D("./Lighthouse_Material/Floor.jpg");  //
        Texture2D lighthouse_wall = Texture2D("./Lighthouse_Material/Windows_Dome - Map.jpg");  //
        Texture2D lighthouse_wood = Texture2D("./Lighthouse_Material/wood2.jpg"); //

        // 在渲染循环中绑定纹理
        /*
        lighthouse_shader_prog.use();
        glActiveTexture(GL_TEXTURE0);  // 激活纹理单元0
        lighthouse_texture.bind();     // 绑定纹理
        lighthouse_shader_prog.setInt("texture1", 0);  // 设置着色器中纹理采样器的值
        */
        
        // 使用灯塔的着色器程序
        lighthouse_shader_prog.use();

        // 绑定iron的纹理
        glActiveTexture(GL_TEXTURE0 + 10);
        lighthouse_iron.bind();
        lighthouseMesh.renderPart("Bl_iron"); // 渲染灯塔的glass部分

        // 绑定blglass的纹理
        glActiveTexture(GL_TEXTURE0 + 11);
        lighthouse_blglass.bind();
        lighthouseMesh.renderPart("bl_glass"); // 渲染灯塔的glass部分

        // 绑定glass的纹理
        glActiveTexture(GL_TEXTURE0 + 12);
        lighthouse_glass.bind();
        lighthouseMesh.renderPart("clglass"); // 渲染灯塔的glass部分

        // 绑定iron的纹理
        glActiveTexture(GL_TEXTURE0 + 13);
        lighthouse_lens.bind();
        lighthouseMesh.renderPart("lens"); // 渲染灯塔的glass部分

        // 绑定iron的纹理
        glActiveTexture(GL_TEXTURE0 + 14);
        lighthouse_lens.bind();
        lighthouseMesh.renderPart("mirror"); // 渲染灯塔的glass部分

        // 绑定iron的纹理
        glActiveTexture(GL_TEXTURE0 + 15);
        lighthouse_rediron.bind();
        lighthouseMesh.renderPart("red_iron"); // 渲染灯塔的glass部分

        // 绑定rock的纹理
        glActiveTexture(GL_TEXTURE0 + 16);
        lighthouse_rock.bind();
        lighthouseMesh.renderPart("rock"); // 渲染灯塔的rock部分

        // 绑定walls的纹理
        glActiveTexture(GL_TEXTURE0 + 17);
        lighthouse_wall.bind();
        lighthouseMesh.renderPart("walls"); // 渲染灯塔的walls部分
        
        // 绑定wood的纹理
        glActiveTexture(GL_TEXTURE0 + 18);
        lighthouse_wood.bind();
        lighthouseMesh.renderPart("wood"); // 渲染灯塔的wood部分
        //*/
        
        //-----------------------//
        // 加载树的模型
        // 加载 tree.obj 文件
        ObjMesh treeMesh = load_wavefront_obj(BASE_PATH + "tree.obj");

        std::vector<Shader> tree_shaders;
        tree_shaders.emplace_back("tree.vert");
        tree_shaders.emplace_back("tree.frag");
        ShaderProgram tree_shader_prog(tree_shaders);

        Texture2D tree_trunk = Texture2D("./tree/bark_0021.jpg");  //
        Texture2D tree_leaf = Texture2D("./tree/DB2X2_L01.png");   //

        // 使用灯塔的着色器程序
        tree_shader_prog.use();

        // 绑定树干的纹理
        glActiveTexture(GL_TEXTURE0 + 20);
        tree_trunk.bind();
        treeMesh.renderPart("Trank_bark"); // 渲染灯塔的glass部分

        // 绑定树叶的纹理
        glActiveTexture(GL_TEXTURE0 + 21);
        tree_leaf.bind();
        treeMesh.renderPart("polySurface1SG1"); // 渲染灯塔的glass部分

        
        //-----------------------//
        // 加载树2的模型
        // 加载 tree8.obj 文件
        ObjMesh tree2Mesh = load_wavefront_obj(BASE_PATH + "tree8.obj");

        std::vector<Shader> tree2_shaders;
        tree2_shaders.emplace_back("tree2.vert");
        tree2_shaders.emplace_back("tree2.frag");
        ShaderProgram tree2_shader_prog(tree2_shaders);

        Texture2D tree2_bark = Texture2D("./tree2/bark.png");  //
        Texture2D tree2_leaf = Texture2D("./tree2/leaf.png");   //

        // 使用灯塔的着色器程序
        tree2_shader_prog.use();

        // 绑定树干的纹理
        glActiveTexture(GL_TEXTURE0 + 22);
        tree2_bark.bind();
        tree2Mesh.renderPart("bark"); // 渲染灯塔的glass部分

        // 绑定树叶的纹理
        glActiveTexture(GL_TEXTURE0 + 23);
        tree2_leaf.bind();
        tree2Mesh.renderPart("leaf"); // 渲染灯塔的glass部分
        //*/

        //-----------------------//
        // 创建球体网格，seabed宽度的1/20
        float seabed_width = CGRA350Constants::DEFAULT_SEABED_GRID_WIDTH;
        std::shared_ptr<SphereMesh> sphere_mesh_ptr = std::make_shared<SphereMesh>(20, 20, seabed_width / 20);
        sphere_mesh_ptr->initialise();

        // 创建球体渲染器
        std::vector<Shader> sphere_shaders;
        sphere_shaders.emplace_back("sphere.vert");
        sphere_shaders.emplace_back("sphere.frag");
        ShaderProgram sphere_shader_prog(sphere_shaders);

        // 渲染球体
        SphereRenderer sphere_renderer(sphere_shader_prog, sphere_mesh_ptr);

        //-----------------------//
        //*/

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
            if (last_env_map != m_context.m_env_map)
            {
                skybox_renderer.setCubeMapTexture(*env_maps[m_context.m_env_map]);
                ocean_renderer_fresnel.setSkyboxTexture(*env_maps[m_context.m_env_map]);
                ocean_renderer_refl.setSkyboxTexture(*env_maps[m_context.m_env_map]);
                last_env_map = m_context.m_env_map;
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

                // render seabed
                if (m_context.m_do_render_seabed)
                {
                    seabed_renderer.render(m_context.m_render_camera);
                }
                // render skybox
                skybox_renderer.render(m_context.m_render_camera);

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

            // --- render skybox ---
            skybox_renderer.render(m_context.m_render_camera);

            // 渲染灯塔模型
            // 设置光照参数
            glm::vec3 light_pos(-15.0f, 85.0f, -15.0f);  // 光源位置
            glm::vec3 light_color(1.0f, 1.0f, 1.0f);    // 光源颜色

            // 渲染灯塔模型
            lighthouse_shader_prog.use();  // 使用灯塔模型的着色器程序
            lighthouse_shader_prog.setVec3("light_pos", light_pos);    // 设置光照位置
            lighthouse_shader_prog.setVec3("light_color", light_color);  // 设置光源颜色
            lighthouse_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // 设置摄像机位置
            lighthouse_shader_prog.setInt("normalMap", 0); // 0表示绑定到的纹理单元

            // 设定灰色材质颜色
            lighthouse_shader_prog.setVec3("object_color", glm::vec3(0.5f, 0.5f, 0.5f));

            // 设置模型矩阵
            glm::mat4 lighthouse_model_matrix = glm::translate(glm::mat4(0.3f), glm::vec3(-80.0f, 15.0f, -420.0f)); // 平移变换
            lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // 沿Z轴顺时针旋转90度
            lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // 沿Z轴顺时针旋转90度
            lighthouse_shader_prog.setMat4("model", lighthouse_model_matrix);

            //glm::mat4 lighthouse_model_matrix = glm::translate(glm::mat4(0.2f), glm::vec3(-20.0f, 0.0f, -160.0f));  // 模型变换
            //lighthouse_shader_prog.setMat4("model", lighthouse_model_matrix);

            // 设置视图和投影矩阵
            lighthouse_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            lighthouse_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // 渲染灯塔模型
            //lighthouseMesh.render();
            //lighthouse_shader_prog.setInt("texture1", 0);  // 设置贴图单元

            lighthouse_shader_prog.setInt("texture1", 10);
            lighthouseMesh.renderPart("Bl_iron");

            lighthouse_shader_prog.setInt("texture1", 11);
            lighthouseMesh.renderPart("bl_glass");

            lighthouse_shader_prog.setInt("texture1", 12);  // 设置贴图单元
            lighthouseMesh.renderPart("clglass");

            lighthouse_shader_prog.setInt("texture1", 13);  // 设置贴图单元
            lighthouseMesh.renderPart("lens");

            lighthouse_shader_prog.setInt("texture1", 14);  // 设置贴图单元
            lighthouseMesh.renderPart("mirror");

            lighthouse_shader_prog.setInt("texture1", 15);  // 设置贴图单元
            lighthouseMesh.renderPart("red_iron");

            lighthouse_shader_prog.setInt("texture1", 16);  // 设置贴图单元
            lighthouseMesh.renderPart("rock");

            lighthouse_shader_prog.setInt("texture1", 17);  // 设置贴图单元
            lighthouseMesh.renderPart("walls");

            lighthouse_shader_prog.setInt("texture1", 18);  // 设置贴图单元
            lighthouseMesh.renderPart("wood");

            //-----------------------------//
            // 渲染树模型
            tree_shader_prog.use();  // 使用灯塔模型的着色器程序
            tree_shader_prog.setVec3("light_pos", light_pos);    // 设置光照位置
            tree_shader_prog.setVec3("light_color", light_color);  // 设置光源颜色
            tree_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // 设置摄像机位置
            tree_shader_prog.setInt("normalMap", 0); // 0表示绑定到的纹理单元

            // 设定灰色材质颜色
            tree_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // 设置模型矩阵
            glm::mat4 tree_model_matrix = glm::translate(glm::mat4(5.0f), glm::vec3(-3.0f, 0.0f, -10.0f));  // 模型变换
            tree_shader_prog.setMat4("model", tree_model_matrix);

            // 设置视图和投影矩阵
            tree_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            tree_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            tree_shader_prog.setInt("texture1", 20);
            treeMesh.renderPart("Trank_bark");

            tree_shader_prog.setInt("texture1", 21);
            treeMesh.renderPart("polySurface1SG1");


            
            //-----------------------------//
            // 渲染树2模型
            tree2_shader_prog.use();  // 使用灯塔模型的着色器程序
            tree2_shader_prog.setVec3("light_pos", light_pos);    // 设置光照位置
            tree2_shader_prog.setVec3("light_color", light_color);  // 设置光源颜色
            tree2_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // 设置摄像机位置
            tree2_shader_prog.setInt("normalMap", 0); // 0表示绑定到的纹理单元

            // 设定灰色材质颜色
            tree2_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // 设置模型矩阵
            glm::mat4 tree2_model_matrix = glm::translate(glm::mat4(1.5f), glm::vec3(-9.0f, 0.0f, -15.0f));  // 模型变换
            tree2_shader_prog.setMat4("model", tree2_model_matrix);

            // 设置视图和投影矩阵
            tree2_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            tree2_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            tree2_shader_prog.setInt("texture1", 22);
            tree2Mesh.renderPart("bark");

            tree2_shader_prog.setInt("texture1", 23);
            tree2Mesh.renderPart("leaf");
            //*/

            // 渲染球体
            // 设置光照和摄像机位置
            //glm::vec3 light_pos(15.0f, -25.0f, 15.0f);
            //glm::vec3 light_color(1.0f, 1.0f, 1.0f);

            // Set light and camera parameters for the sphere
            sphere_renderer.setLight(light_pos, light_color);
            sphere_renderer.setCameraPosition(m_context.m_render_camera.getPosition());
            sphere_renderer.render(m_context.m_render_camera);

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
    }
}
