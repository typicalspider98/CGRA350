// Local Headers
#include "cgra350final.h"
#include "constants.h"
#include "../graphics/shaders.h"
#include "../graphics/renderers.h"
#include "../graphics/textures.h"
#include "../graphics/postprocessing.h"
#include "../volumerendering/vector.cuh"
#include "../computeinstancing/Rain.hpp"

#include <cuda_runtime.h>
#include <vector_types.h>
#include <glm/gtc/type_ptr.hpp>


// System Headers
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

const std::string BASE_PATH = "../resources/assets/";
const std::string BASE_PATH_TEXTURE = "../resources/textures/";
const std::string LIGHTHOUSE_TEXTURE = "../resources/textures/lighthouse/";

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
        if (m_context.m_enable_cloud_func)
        {
            CheckCuda(m_context.m_gui_param.device_name);

            string cloud_path = "./../data/CLOUD0";
            m_volumerender = new VolumeRender(cloud_path);
            InitCloud(m_context.m_render_camera, *m_volumerender, m_context.m_gui_param);
        }
    }

    //！！！！！！！！！！！！！！！！！！！！！！！！！！！！//
    // Load obj files
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

        // Parse the.obj file
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

                // Treated triangular face
                if (face.size() == 3) {
                    for (int i = 0; i < 3; ++i) {
                        wv_vertices.push_back(face[i]);
                    }

                    for (int i = 0; i < 3; ++i) {
                        // Adds the face to the corresponding part of the current material
                        meshParts[currentMaterialName].positions.push_back(positions[face[i].p]);
                        meshParts[currentMaterialName].normals.push_back(normals[face[i].n]);
                        meshParts[currentMaterialName].texCoords.push_back(uvs[face[i].t]);
                        meshParts[currentMaterialName].indices.push_back(meshParts[currentMaterialName].positions.size() - 1);
                    }
                }
            }
            else if (mode == "usemtl") {
                // Handle new materials
                objLine >> currentMaterialName;
            }
        }

        // Generating normal
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

        // Build index
        for (const wavefront_vertex& v : wv_vertices) {
            indices.push_back(v.p);
        }

        // Create an ObjMesh object
        ObjMesh objMesh(positions, normals, uvs, indices);
        objMesh.initialise();  // Initializes OpenGL data

        for (auto& part : meshParts) {
            part.second.initialise();
            objMesh.parts[part.first] = part.second;
        }

        return objMesh;
    }
    //！！！！！！！！！！！！！！！！！！！！！！！！！！！！//

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
                            m_context.m_gui_param.raindrop_max_speed,
                            m_context.m_gui_param.rain_sea_level);

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

        //！！！！！！！！！！！！！！！！！！！！！！！！！！！！//
        // OBJ processing

        //-----------------------//
        // Loaded lighthouse model
        ObjMesh lighthouseMesh = load_wavefront_obj(BASE_PATH + "lighthouse9.obj");

        std::vector<Shader> lighthouse_shaders;

        if (m_context.m_light_model == 0) {
            lighthouse_shaders.emplace_back("lighthouse.vert");
            lighthouse_shaders.emplace_back("lighthouse.frag");
        }
        else if (m_context.m_light_model == 1) {
            lighthouse_shaders.emplace_back("lighthouse_CookTorrance.vert");
            lighthouse_shaders.emplace_back("lighthouse_CookTorrance.frag");
        }
        else if (m_context.m_light_model == 2) {
            lighthouse_shaders.emplace_back("lighthouse_OrenNayar.vert");
            lighthouse_shaders.emplace_back("lighthouse_OrenNayar.frag");
        }

        ShaderProgram lighthouse_shader_prog(lighthouse_shaders);

        // Load normal maps or color maps for each part of the lighthouse

        Texture2D lighthouse_wall = Texture2D("./Lighthouse_Material/Windows_Dome - Map.jpg");
        Texture2D lighthouse_iron = Texture2D("./Lighthouse_Material/Floor2.jpg");
        Texture2D lighthouse_blglass = Texture2D("./Lighthouse_Material/window4.jpg");
        Texture2D lighthouse_glass = Texture2D("./Lighthouse_Material/Wooden_door.jpg");
        Texture2D lighthouse_lens = Texture2D("./Lighthouse_Material/Handle0.jpg");
        Texture2D lighthouse_mirror = Texture2D("./Lighthouse_Material/window2.jpg");
        Texture2D lighthouse_rediron = Texture2D("./Lighthouse_Material/roof2.jpg");
        Texture2D lighthouse_rock = Texture2D("./Lighthouse_Material/Floor.jpg");
        Texture2D lighthouse_wood = Texture2D("./Lighthouse_Material/wood2.jpg");

        Texture2D lighthouse_wall1 = Texture2D("./Lighthouse_Material/Windows_Dome - Map.jpg");
        Texture2D lighthouse_wall2 = Texture2D("./Lighthouse_Material/wood2.jpg");
        Texture2D lighthouse_wall3 = Texture2D("./Lighthouse_Material/25_concrete.png");
        Texture2D lighthouse_wall4 = Texture2D("./Lighthouse_Material/27_grey_new_brick.png");
        Texture2D lighthouse_wall5 = Texture2D("./Lighthouse_Material/28_grey_marble.png");
        Texture2D lighthouse_wall6 = Texture2D("./Lighthouse_Material/30_stainless steel.jpeg");
        Texture2D lighthouse_wall7 = Texture2D("./Lighthouse_Material/31_brushed_medal.jpg");

        Texture2D lighthouse_roof1 = Texture2D("./Lighthouse_Material/roof2.jpg");
        Texture2D lighthouse_roof2 = Texture2D("./Lighthouse_Material/16_medal.jpg");
        Texture2D lighthouse_roof3 = Texture2D("./Lighthouse_Material/18_marble.jpg");
        Texture2D lighthouse_roof4 = Texture2D("./Lighthouse_Material/23_rusty_medal.jpg");
        Texture2D lighthouse_roof5 = Texture2D("./Lighthouse_Material/25_concrete.png");
        Texture2D lighthouse_roof6 = Texture2D("./Lighthouse_Material/30_stainless steel.jpeg");
        Texture2D lighthouse_roof7 = Texture2D("./Lighthouse_Material/33_glavanized_medal.jpg");
        Texture2D lighthouse_roof8 = Texture2D("./Lighthouse_Material/35_yellow_wood.jpg");

        Texture2D lighthouse_rock1 = Texture2D("./Lighthouse_Material/Floor.jpg");
        Texture2D lighthouse_rock2 = Texture2D("./Lighthouse_Material/11_soil.jpg");
        Texture2D lighthouse_rock3 = Texture2D("./Lighthouse_Material/17_sand.jpg");
        Texture2D lighthouse_rock4 = Texture2D("./Lighthouse_Material/18_marble.jpg");
        Texture2D lighthouse_rock5 = Texture2D("./Lighthouse_Material/19_black stone.jpg");
        Texture2D lighthouse_rock6 = Texture2D("./Lighthouse_Material/21_cobblestone.png");
        Texture2D lighthouse_rock7 = Texture2D("./Lighthouse_Material/22_medal.png");
        Texture2D lighthouse_rock8 = Texture2D("./Lighthouse_Material/34_concrete.jpeg");
        Texture2D lighthouse_rock9 = Texture2D("./Lighthouse_Material/35_yellow_wood.jpg");

        lighthouse_shader_prog.setFloat("roughness", m_context.m_lighthouse_roughness);
        lighthouse_shader_prog.setFloat("metalness", m_context.m_lighthouse_medalness);
        lighthouse_shader_prog.setFloat("reflectivity", m_context.m_lighthouse_reflectivity);

        // Use the lighthouse shader program
        lighthouse_shader_prog.use();
        glActiveTexture(GL_TEXTURE0);

        // Bind iron's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_IRON);
        lighthouse_iron.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_IRON);

        // Bind blglass's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_BLGLASS);
        lighthouse_blglass.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_BLGLASS);

        // Bind glass's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_GLASS);
        lighthouse_glass.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_GLASS);

        //  Bind lens's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_LENS);
        lighthouse_lens.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_LENS);

        //  Bind mirror's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_MIRROR);
        lighthouse_mirror.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_MIRROR);

        // Bind rediron's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_REDIRON);
        lighthouse_rediron.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_REDIRON);

        // Bind rock's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_ROCK);
        lighthouse_rock.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_ROCK);

        // Bind wall's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WALL);
        lighthouse_wall.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WALL);

        // Bind wood's texture
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WOOD);
        lighthouse_wood.bind();
        lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WOOD);
        //*/

        //-----------------------//
        // Load the tree model
        ObjMesh treeMesh = load_wavefront_obj(BASE_PATH + "tree.obj");

        std::vector<Shader> trunk_shaders;
        trunk_shaders.emplace_back("tree.vert");
        trunk_shaders.emplace_back("tree.frag");
        ShaderProgram trunk_shader_prog(trunk_shaders);

        std::vector<Shader> leaf_shaders;
        leaf_shaders.emplace_back("tree.vert");
        leaf_shaders.emplace_back("tree_leaf.frag");
        ShaderProgram leaf_shader_prog(leaf_shaders);

        Texture2D tree_trunk = Texture2D("./tree/bark_0021.jpg");
        Texture2D tree_leaf = Texture2D("./tree/DB2X2_L01.png");
        Texture2D tree_leaf_normal_map = Texture2D("./tree/DB2X2_L01_Nor.png");
        Texture2D tree_leaf_specular_map = Texture2D("./tree/DB2X2_L01_Spec.png");

        // Shader program using trunk
        trunk_shader_prog.use();
        // Bind the texture of the trunk
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
        tree_trunk.bind();
        trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
        
        // Shader program using leaves 
        leaf_shader_prog.use();

        // Bind the basic color map of the leaves      
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
        tree_leaf.bind();
        leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
        // Bind the normal map of the leaves
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
        tree_leaf_normal_map.bind();  
        leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
        // Bind specific light maps to leaves
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
        tree_leaf_specular_map.bind(); 
        leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);

        //*/
        //-----------------------//
        // Load the tree2 model
        ObjMesh tree2Mesh = load_wavefront_obj(BASE_PATH + "tree8.obj");

        Texture2D tree2_bark = Texture2D("./tree2/bark.png");  //
        Texture2D tree2_leaf = Texture2D("./tree2/leaf.png");   //
        Texture2D tree2_leaf_normal_map = Texture2D("./tree2/leaf_normal.png");   //
        Texture2D tree2_leaf_specular_map = Texture2D("./tree2/leaf_specular.png");   //

        // Shader program using trunk
        trunk_shader_prog.use();
        // Bind the texture of the trunk
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
        tree2_bark.bind();
        trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);

        // Shader program using leaf
        leaf_shader_prog.use();
        // Bind the texture of the leaves
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
        tree2_leaf.bind();
        leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
        // Bind the normal map of the leaves
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
        tree2_leaf_normal_map.bind();
        leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
        // Bind specific light maps to leaves
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
        tree2_leaf_specular_map.bind();
        leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);

        //*/
        // Load a bunch of stone models
        ObjMesh rocksMesh = load_wavefront_obj(BASE_PATH + "rocks.obj");

        std::vector<Shader> rocks_shaders;
        rocks_shaders.emplace_back("rocks.vert");
        rocks_shaders.emplace_back("rocks.frag");
        ShaderProgram rocks_shader_prog(rocks_shaders);

        Texture2D rocks_texture = Texture2D("./rocks/Handle0.jpg");  //

        // Use rock shader program
        rocks_shader_prog.use();

        // Bind the texture of the rock
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_ROCKS);
        rocks_texture.bind();
        rocksMesh.renderPart("AssortedRocks");
        //*/

        // Load the large stone model
        ObjMesh caverockMesh = load_wavefront_obj(BASE_PATH + "caverock.obj");

        std::vector<Shader> caverock_shaders;
        caverock_shaders.emplace_back("caverock.vert");
        caverock_shaders.emplace_back("caverock.frag");
        ShaderProgram caverock_shader_prog(caverock_shaders);

        Texture2D caverock_texture = Texture2D("./rocks/Ground.jpg");  //

        // Shader program using big stones
        caverock_shader_prog.use();

        // Bind the texture of large stones
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_CAVEROCK);
        caverock_texture.bind();
        caverockMesh.renderPart("CavePlatform4");
        //*/

        // Load the normal stone model
        ObjMesh stoneMesh = load_wavefront_obj(BASE_PATH + "SmallArch_Obj.obj");
        //ObjMesh stoneMesh = load_wavefront_obj(BASE_PATH + "CaveWalls4_B.obj");

        std::vector<Shader> stone_shaders;
        stone_shaders.emplace_back("stone.vert");
        stone_shaders.emplace_back("stone.frag");
        ShaderProgram stone_shader_prog(stone_shaders);

        Texture2D stone_texture = Texture2D("./stone/DSC_4736.jpg");  //

        //Shader program using ordinary stones
        stone_shader_prog.use();

        // Bind the texture of ordinary stones
        glActiveTexture(GL_TEXTURE0 + 1);
        stone_texture.bind();
        stoneMesh.render();
        //*/

        // Load normal stone 2 model
        ObjMesh stone2Mesh = load_wavefront_obj(BASE_PATH + "CaveWalls4_B.obj");

        std::vector<Shader> stone2_shaders;
        stone2_shaders.emplace_back("stone.vert");
        stone2_shaders.emplace_back("stone.frag");
        ShaderProgram stone2_shader_prog(stone_shaders);

        Texture2D stone2_texture = Texture2D("./Lighthouse_Material/13_stone2_iron.jpg");  //

        // Shader program using ordinary stones
        stone2_shader_prog.use();

        // Bind the texture of ordinary stones
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_STONE2);
        stone2_texture.bind();
        stone2Mesh.render();

        // ------------------------------
        // Postprocessing
        std::vector<Shader> postprocessing_shaders;
        postprocessing_shaders.emplace_back("postprocessing.vert");
        postprocessing_shaders.emplace_back("postprocessing.frag");
        ShaderProgram postprocessing_shader_prog(postprocessing_shaders);
        Postprocessing postprocessing(postprocessing_shader_prog, m_window.getScreenWidth(), m_window.getScreenHeight());


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
       
            // --- Camera Parameters
            const glm::mat proj = m_context.m_render_camera.getProjMatrix();
            const glm::mat view = m_context.m_render_camera.getViewMatrix();
            const glm::vec3 cameraRight = m_context.m_render_camera.getRightVector();
            const glm::vec3 cameraUp = m_context.m_render_camera.getUpVector();

            // --- Directional Light Parameters
            glm::vec3 dLightDirection = m_context.m_gui_param.GetDLight_Direction();
            glm::vec3 dLightColour(m_context.m_gui_param.dlight_color.x, m_context.m_gui_param.dlight_color.y, m_context.m_gui_param.dlight_color.z);
            float dLightStrength = m_context.m_gui_param.dlight_strength;

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
                case 0: {
                    // The uniform variable is set in the render loop
                    ocean_shader_prog_fresnel.use();
                    ocean_shader_prog_fresnel.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_fresnel.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_fresnel.setFloat("light.strength", dLightStrength);

                    ocean_renderer_fresnel.render(m_context.m_render_camera);
                }
                    break;
                case 1: {
                    // The uniform variable is set in the render loop
                    ocean_shader_prog_refl.use();
                    ocean_shader_prog_refl.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_refl.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_refl.setFloat("light.strength", dLightStrength);

                    ocean_renderer_refl.render(m_context.m_render_camera);
                }
                    break;
                case 2: {
                    // The uniform variable is set in the render loop
                    ocean_shader_prog_refr.use();
                    ocean_shader_prog_refr.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_refr.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_refr.setFloat("light.strength", dLightStrength);
                    
                    ocean_renderer_refr.render(m_context.m_render_camera);
                }
                    break;
                default:
                {
                    // The uniform variable is set in the render loop
                    ocean_shader_prog_phong.use();
                    ocean_shader_prog_phong.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_phong.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_phong.setFloat("light.strength", dLightStrength);

                    ocean_renderer_phong.render(m_context.m_render_camera);
                }
                    break;
                }
            }

            // --- render seabed ---
            seabed_shader_prog.use();
            seabed_shader_prog.setVec3("light.direction", dLightDirection);
            seabed_shader_prog.setVec3("light.colour", dLightColour);
            seabed_shader_prog.setFloat("light.strength", dLightStrength);

            if (m_context.m_do_render_seabed)
            {
                seabed_renderer.render(m_context.m_render_camera);
            }

            // --- compute Rain and render Rain Splashes ---
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
                        m_context.m_gui_param.raindrop_max_speed,
                        m_context.m_gui_param.rain_sea_level);
                }

                rain.computeRainOnGPU(ImGui::GetIO().DeltaTime,
                    m_context.m_gui_param.rain_sea_level,
                    m_context.m_gui_param.rain_position,
                    m_context.m_gui_param.rain_radius,
                    m_context.m_gui_param.raindrop_min_speed,
                    m_context.m_gui_param.raindrop_max_speed);
                rain.renderSplashes(proj, view, cameraRight, cameraUp, ImGui::GetIO().DeltaTime);
            }

            if (m_context.m_appear_lighthouse == true) {
                //-----------------------------//
                std::vector<Shader> lighthouse_shaders;
                if (m_context.m_light_model == 0) {
                    lighthouse_shaders.emplace_back("lighthouse.vert");
                    lighthouse_shaders.emplace_back("lighthouse.frag");
                }
                else if (m_context.m_light_model == 1) {
                    lighthouse_shaders.emplace_back("lighthouse_CookTorrance.vert");
                    lighthouse_shaders.emplace_back("lighthouse_CookTorrance.frag");
                }
                else if (m_context.m_light_model == 2) {
                    lighthouse_shaders.emplace_back("lighthouse_OrenNayar.vert");
                    lighthouse_shaders.emplace_back("lighthouse_OrenNayar.frag");
                }
                ShaderProgram lighthouse_shader_prog(lighthouse_shaders);

                // Render lighthouse model
                lighthouse_shader_prog.use();  // Shader program using lighthouse model

                lighthouse_shader_prog.setFloat("roughness", m_context.m_lighthouse_roughness);
                lighthouse_shader_prog.setFloat("metalness", m_context.m_lighthouse_medalness);
                lighthouse_shader_prog.setFloat("reflectivity", m_context.m_lighthouse_reflectivity);

                lighthouse_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                lighthouse_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                lighthouse_shader_prog.setFloat("light.strength", dLightStrength);

                lighthouse_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // Set camera position
                lighthouse_shader_prog.setInt("normalMap", 0); // 0 represents the texture unit to which it is bound

                // Set the gray material color
                lighthouse_shader_prog.setVec3("object_color", glm::vec3(0.5f, 0.5f, 0.5f));

                // Set model matrix
                glm::mat4 lighthouse_model_matrix = glm::translate(glm::mat4(0.3f), glm::vec3(-80.0f, 15.0f, -420.0f)); // Translation transformation
                lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate 90 degrees clockwise along the X axis
                lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate 90 degrees clockwise along the Z axis
                lighthouse_shader_prog.setMat4("model", lighthouse_model_matrix);

                // Set the view and projection matrix
                lighthouse_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
                lighthouse_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

                // Modify Wall map
                if (m_context.m_wall_material == 0) {
                    lighthouse_wall = lighthouse_wall1;  //
                }
                else if (m_context.m_wall_material == 1) {
                    lighthouse_wall = lighthouse_wall2;  //
                }
                else if (m_context.m_wall_material == 2) {
                    lighthouse_wall = lighthouse_wall3;  //
                }
                else if (m_context.m_wall_material == 3) {
                    lighthouse_wall = lighthouse_wall4;  //
                }
                else if (m_context.m_wall_material == 4) {
                    lighthouse_wall = lighthouse_wall5;  //
                }
                else if (m_context.m_wall_material == 5) {
                    lighthouse_wall = lighthouse_wall6;  //
                }
                else if (m_context.m_wall_material == 6) {
                    lighthouse_wall = lighthouse_wall7;  //
                }

                // Modify Roof map
                if (m_context.m_roof_material == 0) {
                    lighthouse_rediron = lighthouse_roof1;  //
                }
                else if (m_context.m_roof_material == 1) {
                    lighthouse_rediron = lighthouse_roof2;  //
                }
                else if (m_context.m_roof_material == 2) {
                    lighthouse_rediron = lighthouse_roof3;  //
                }
                else if (m_context.m_roof_material == 3) {
                    lighthouse_rediron = lighthouse_roof4;  //
                }
                else if (m_context.m_roof_material == 4) {
                    lighthouse_rediron = lighthouse_roof5;  //
                }
                else if (m_context.m_roof_material == 5) {
                    lighthouse_rediron = lighthouse_roof6;  //
                }
                else if (m_context.m_roof_material == 6) {
                    lighthouse_rediron = lighthouse_roof7;  //
                }
                else if (m_context.m_roof_material == 7) {
                    lighthouse_rediron = lighthouse_roof8;  //
                }

                // Modify Rock map
                if (m_context.m_bottom_material == 0) {
                    lighthouse_rock = lighthouse_rock1;  //
                }
                else if (m_context.m_bottom_material == 1) {
                    lighthouse_rock = lighthouse_rock2;  //
                }
                else if (m_context.m_bottom_material == 2) {
                    lighthouse_rock = lighthouse_rock3;  //
                }
                else if (m_context.m_bottom_material == 3) {
                    lighthouse_rock = lighthouse_rock4;  //
                }
                else if (m_context.m_bottom_material == 4) {
                    lighthouse_rock = lighthouse_rock5;  //
                }
                else if (m_context.m_bottom_material == 5) {
                    lighthouse_rock = lighthouse_rock6;  //
                }
                else if (m_context.m_bottom_material == 6) {
                    lighthouse_rock = lighthouse_rock7;  //
                }
                else if (m_context.m_bottom_material == 7) {
                    lighthouse_rock = lighthouse_rock8;  //
                }
                else if (m_context.m_bottom_material == 8) {
                    lighthouse_rock = lighthouse_rock9;  //
                }

                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_IRON);
                lighthouse_iron.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_IRON);
                lighthouseMesh.renderPart("Bl_iron");

                // Render the bl_glass section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_BLGLASS);
                lighthouse_blglass.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_BLGLASS);
                lighthouseMesh.renderPart("bl_glass");

                // Render the clglass section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_GLASS);
                lighthouse_glass.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_GLASS);
                lighthouseMesh.renderPart("clglass");

                // Render the lens section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_LENS);
                lighthouse_lens.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_LENS);
                lighthouseMesh.renderPart("lens");

                // Render the mirror section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_MIRROR);
                lighthouse_mirror.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_MIRROR);
                lighthouseMesh.renderPart("mirror");

                // Render the red_iron section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_REDIRON);
                lighthouse_rediron.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_REDIRON);
                lighthouseMesh.renderPart("red_iron");

                // Render the rock section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_ROCK);
                lighthouse_rock.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_ROCK);
                lighthouseMesh.renderPart("rock");

                // Render the wall section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WALL);
                lighthouse_wall.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WALL);
                lighthouseMesh.renderPart("walls");

                // Render the wood section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WOOD);
                lighthouse_wood.bind();
                lighthouse_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_LIGHTHOUSE_WOOD);
                lighthouseMesh.renderPart("wood");
            }

            //-----------------------------//
            if (m_context.m_appear_tree == true) {
                // Render tree 1 model
                trunk_shader_prog.use();

                trunk_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                trunk_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                trunk_shader_prog.setFloat("light.strength", dLightStrength);

                trunk_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // Set camera position
                // Set model matrix
                glm::mat4 tree_model_matrix = glm::translate(glm::mat4(5.0f), glm::vec3(-5.0f, -1.1f, -12.0f));
                tree_model_matrix = glm::scale(tree_model_matrix, glm::vec3(0.6f, 0.6f, 0.6f));
                trunk_shader_prog.setMat4("model", tree_model_matrix);
                // Set the view and projection matrix
                trunk_shader_prog.setMat4("view", view);
                trunk_shader_prog.setMat4("projection", proj);
                // Render the trunk section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
                tree_trunk.bind();
                trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
                treeMesh.renderPart("bark");

                // Render the leaf section
                leaf_shader_prog.use();

                leaf_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                leaf_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                leaf_shader_prog.setFloat("light.strength", dLightStrength);

                leaf_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // Set camera position
                // Set model matrix
                leaf_shader_prog.setMat4("model", tree_model_matrix);
                // Set the view and projection matrix
                leaf_shader_prog.setMat4("view", view);
                leaf_shader_prog.setMat4("projection", proj);
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
                tree_leaf.bind();
                leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
                // Bind the normal map of the leaves
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
                tree_leaf_normal_map.bind();
                leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
                // Bind specific light maps to leaves
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
                tree_leaf_specular_map.bind();
                leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
                treeMesh.renderPart("leaf");
                //*/
            }

            if (m_context.m_appear_tree == true) {
                //-----------------------------//
                // Render tree 2 model
                trunk_shader_prog.use();

                trunk_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                trunk_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                trunk_shader_prog.setFloat("light.strength", dLightStrength);

                trunk_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // Set camera position
                // Set model matrix
                glm::mat4 tree2_model_matrix = glm::translate(glm::mat4(1.5f), glm::vec3(-18.0f, -3.5f, -25.0f));
                trunk_shader_prog.setMat4("model", tree2_model_matrix);
                // Set the view and projection matrix
                trunk_shader_prog.setMat4("view", view);
                trunk_shader_prog.setMat4("projection", proj);
                // Render the trunk section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
                tree2_bark.bind();
                trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
                tree2Mesh.renderPart("bark");

                // Render the leaf section
                leaf_shader_prog.use();
                leaf_shader_prog.setVec3("light_dir", dLightDirection); // Set light position
                leaf_shader_prog.setVec3("light_color", dLightColour); // Set light color
                leaf_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // Set camera position
                // Set the view and projection matrix
                leaf_shader_prog.setMat4("model", tree2_model_matrix);
                leaf_shader_prog.setMat4("view", view);
                leaf_shader_prog.setMat4("projection", proj);
                // Bind the texture of the leaves
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
                tree2_leaf.bind();
                leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
                // Bind the normal map of the leaves
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
                tree2_leaf_normal_map.bind();
                leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
                // Bind specific light maps to leaves
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
                tree2_leaf_specular_map.bind();
                leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
                tree2Mesh.renderPart("leaf");
                //*/
            }

            if (m_context.m_appear_stone == true) {
                //-----------------------------//
                // Render rock model
                rocks_shader_prog.use();  // Shader program using lighthouse model

                rocks_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                rocks_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                rocks_shader_prog.setFloat("light.strength", dLightStrength);

                rocks_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // Set camera position
                rocks_shader_prog.setInt("normalMap", 0); // 0 represents the texture unit to which it is bound

                // Set the stone material color
                rocks_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

                // Set model matrix
                glm::mat4 rocks_model_matrix = glm::translate(glm::mat4(0.9f), glm::vec3(-280.0f, -37.0f, -180.0f));
                rocks_model_matrix = glm::rotate(rocks_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 90 degrees clockwise along the Y axis
                rocks_shader_prog.setMat4("model", rocks_model_matrix);

                // Set the view and projection matrix
                rocks_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
                rocks_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

                // Render stone section
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_ROCKS);
                rocks_texture.bind();
                rocks_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_ROCKS);
                rocksMesh.renderPart("AssortedRocks");
                //*/
            }

            if (m_context.m_appear_stone == true) {
                //-----------------------------//
                // Render large stone models
                caverock_shader_prog.use();  // Shader program using large stone models

                caverock_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                caverock_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                caverock_shader_prog.setFloat("light.strength", dLightStrength);

                caverock_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // Set camera position
                caverock_shader_prog.setInt("normalMap", 0); // 0 represents the texture unit to which it is bound

                // Set the big stone material color
                caverock_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

                // Set model matrix
                glm::mat4 caverock_model_matrix = glm::translate(glm::mat4(0.1f), glm::vec3(-120.0f, -42.0f, -150.0f));
                caverock_model_matrix = glm::rotate(caverock_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 90 degrees clockwise along the Y axis
                caverock_shader_prog.setMat4("model", caverock_model_matrix);

                // Set the view and projection matrix
                caverock_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
                caverock_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

                // Render large stone parts
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_CAVEROCK);
                caverock_texture.bind();
                caverock_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_CAVEROCK);
                caverockMesh.renderPart("CavePlatform4");
                //*/
            }

            if (m_context.m_appear_stone == true) {
                //-----------------------------//
                // Render normal stone models
                stone_shader_prog.use();  // Shader program using ordinary stone models

                stone_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                stone_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                stone_shader_prog.setFloat("light.strength", dLightStrength);

                stone_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // Set camera position
                stone_shader_prog.setInt("normalMap", 0); // 0 represents the texture unit to which it is bound

                // Set the normal stone material color
                stone_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

                // Set model matrix
                glm::mat4 stone_model_matrix = glm::translate(glm::mat4(0.2f), glm::vec3(-340.0f, -25.0f, -180.0f));
                stone_model_matrix = glm::rotate(stone_model_matrix, glm::radians(-100.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 90 degrees clockwise along the Y axis
                stone_shader_prog.setMat4("model", stone_model_matrix);

                // Set the view and projection matrix
                stone_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
                stone_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

                // Render normal stone parts
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_STONE);
                stone_texture.bind();
                stone_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_STONE);
                stoneMesh.render();
                //*/
            }

            if (m_context.m_appear_stone == true) {
                //-----------------------------//
                // Render normal stone 2 models
                stone2_shader_prog.use();  // Shader program using ordinary stone models

                stone2_shader_prog.setVec3("light.direction", dLightDirection);    // Set light position
                stone2_shader_prog.setVec3("light.colour", dLightColour);  // Set light color
                stone2_shader_prog.setFloat("light.strength", dLightStrength);

                stone2_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // Set camera position
                stone2_shader_prog.setInt("normalMap", 0); // 0 represents the texture unit to which it is bound

                // Set the normal stone material color
                stone2_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

                // Set model matrix
                glm::mat4 stone2_model_matrix = glm::translate(glm::mat4(0.6f), glm::vec3(-24.0f, -7.0f, -380.0f));
                stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate 90 degrees clockwise along the X axis
                stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate 90 degrees clockwise along the Z axis
                stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 90 degrees clockwise along the Y axis
                stone2_shader_prog.setMat4("model", stone2_model_matrix);

                // Set the view and projection matrix
                stone2_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
                stone2_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

                // Render normal stone parts
                glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_STONE2);
                stone2_texture.bind();
                stone2_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_STONE2);
                stone2Mesh.render();
                //*/
            }
            
            //！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！//

            // --- render Rain Drops ---
            if (m_context.m_do_render_rain)
            {
                rain.renderRaindrops(proj, view, ImGui::GetIO().DeltaTime,
                    m_context.m_gui_param.raindrop_length,
                    m_context.m_gui_param.raindrop_color);
            }

            // --- render cloud ---
            if (m_context.m_do_render_cloud)
            {
                RenderCloud(m_context.m_render_camera, *m_volumerender, m_window.getWindow());
            }

            // --- postprocessing
            if (m_context.m_do_render_postprocessing)
            {
                postprocessing.blitFrameBuffer(m_window.getScreenWidth(), m_window.getScreenHeight());
                postprocessing.beforeRender(m_window.getScreenWidth(), m_window.getScreenHeight());
                postprocessing.render(m_context.m_gui_param);
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
