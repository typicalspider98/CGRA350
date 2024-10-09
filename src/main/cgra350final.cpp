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
        if (m_context.m_do_render_cloud)
        {
            CheckCuda();

            string cloud_path = "./../data/CLOUD0";
            m_volumerender = new VolumeRender(cloud_path);
            InitCloud(m_context.m_render_camera, *m_volumerender, m_context.m_gui_param);
        }
    }

    //��������������������������������������������������������//
    // ����obj
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

        // ���� .obj �ļ�
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

                // ������������
                if (face.size() == 3) {
                    for (int i = 0; i < 3; ++i) {
                        wv_vertices.push_back(face[i]);
                    }

                    for (int i = 0; i < 3; ++i) {
                        // ������뵱ǰ���ʶ�Ӧ�Ĳ���
                        meshParts[currentMaterialName].positions.push_back(positions[face[i].p]);
                        meshParts[currentMaterialName].normals.push_back(normals[face[i].n]);
                        meshParts[currentMaterialName].texCoords.push_back(uvs[face[i].t]);
                        meshParts[currentMaterialName].indices.push_back(meshParts[currentMaterialName].positions.size() - 1);
                    }
                }
            }
            else if (mode == "usemtl") {
                // �����µĲ���
                objLine >> currentMaterialName;
            }
        }

        // ���ɷ���
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

        // ��������
        for (const wavefront_vertex& v : wv_vertices) {
            indices.push_back(v.p);
        }

        // ���� ObjMesh ����
        ObjMesh objMesh(positions, normals, uvs, indices);
        objMesh.initialise();  // ��ʼ�� OpenGL ����

        for (auto& part : meshParts) {
            part.second.initialise();
            objMesh.parts[part.first] = part.second;
        }

        return objMesh;
    }
    //��������������������������������������������������������//

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

        //��������������������������������������������������������//
        //OBJ����

        //-----------------------//
        // ���ص���ģ��
        // ���� teapot.obj �ļ�
        ObjMesh lighthouseMesh = load_wavefront_obj(BASE_PATH + "lighthouse9.obj");

        std::vector<Shader> lighthouse_shaders;
        lighthouse_shaders.emplace_back("lighthouse.vert");
        lighthouse_shaders.emplace_back("lighthouse.frag");
        ShaderProgram lighthouse_shader_prog(lighthouse_shaders);

        // ���ص���ǽ�ڵķ�����ͼ����ɫ��ͼ
        //Texture2D lighthouse_texture = Texture2D("./lighthouse_wall4.jpg");  // ��������

        Texture2D lighthouse_iron = Texture2D("./Lighthouse_Material/Floor2.jpg");  //
        Texture2D lighthouse_blglass = Texture2D("./Lighthouse_Material/window4.jpg");   //
        Texture2D lighthouse_glass = Texture2D("./Lighthouse_Material/Wooden_door.jpg");  //
        Texture2D lighthouse_lens = Texture2D("./Lighthouse_Material/Handle0.jpg");
        Texture2D lighthouse_mirror = Texture2D("./Lighthouse_Material/window2.jpg");  //
        Texture2D lighthouse_rediron = Texture2D("./Lighthouse_Material/roof2.jpg"); // roof
        Texture2D lighthouse_rock = Texture2D("./Lighthouse_Material/Floor.jpg");  //
        Texture2D lighthouse_wall = Texture2D("./Lighthouse_Material/Windows_Dome - Map.jpg");  //
        Texture2D lighthouse_wood = Texture2D("./Lighthouse_Material/wood2.jpg"); //

        // ����Ⱦѭ���а�����
        /*
        lighthouse_shader_prog.use();
        glActiveTexture(GL_TEXTURE0);  // ��������Ԫ0
        lighthouse_texture.bind();     // ������
        lighthouse_shader_prog.setInt("texture1", 0);  // ������ɫ���������������ֵ
        */

        // ʹ�õ�������ɫ������
        lighthouse_shader_prog.use();
        glActiveTexture(GL_TEXTURE0);

        // ��iron������
        glActiveTexture(GL_TEXTURE0 + 5);
        lighthouse_iron.bind();
        lighthouse_shader_prog.setInt("texture1", 5);
        //lighthouseMesh.renderPart("Bl_iron"); // ��Ⱦ������glass����

        // ��blglass������
        glActiveTexture(GL_TEXTURE0 + 6);
        lighthouse_blglass.bind();
        lighthouse_shader_prog.setInt("texture1", 6);
        //lighthouseMesh.renderPart("bl_glass"); // ��Ⱦ������glass����

        // ��glass������
        glActiveTexture(GL_TEXTURE0 + 7);
        lighthouse_glass.bind();
        lighthouse_shader_prog.setInt("texture1", 7);
        //lighthouseMesh.renderPart("clglass"); // ��Ⱦ������glass����

        // ��iron������
        glActiveTexture(GL_TEXTURE0 + 8);
        lighthouse_lens.bind();
        lighthouse_shader_prog.setInt("texture1", 8);
        //lighthouseMesh.renderPart("lens"); // ��Ⱦ������glass����

        // ��iron������
        glActiveTexture(GL_TEXTURE0 + 9);
        lighthouse_lens.bind();
        lighthouse_shader_prog.setInt("texture1", 9);
        //lighthouseMesh.renderPart("mirror"); // ��Ⱦ������glass����

        // ��iron������
        glActiveTexture(GL_TEXTURE0 + 10);
        lighthouse_rediron.bind();
        lighthouse_shader_prog.setInt("texture1", 10);
        //lighthouseMesh.renderPart("red_iron"); // ��Ⱦ������glass����

        // ��rock������
        glActiveTexture(GL_TEXTURE0 + 11);
        lighthouse_rock.bind();
        lighthouse_shader_prog.setInt("texture1", 11);
        //lighthouseMesh.renderPart("rock"); // ��Ⱦ������rock����

        // ��walls������
        glActiveTexture(GL_TEXTURE0 + 12);
        lighthouse_wall.bind();
        lighthouse_shader_prog.setInt("texture1", 12);
        //lighthouseMesh.renderPart("walls"); // ��Ⱦ������walls����

        // ��wood������
        glActiveTexture(GL_TEXTURE0 + 13);
        lighthouse_wood.bind();
        lighthouse_shader_prog.setInt("texture1", 13);
        //lighthouseMesh.renderPart("wood"); // ��Ⱦ������wood����
        //*/


        //-----------------------//
        // ��������ģ��
        // ���� tree.obj �ļ�
        ObjMesh treeMesh = load_wavefront_obj(BASE_PATH + "tree.obj");

        std::vector<Shader> trunk_shaders;
        trunk_shaders.emplace_back("tree.vert");
        trunk_shaders.emplace_back("tree.frag");
        ShaderProgram trunk_shader_prog(trunk_shaders);

        std::vector<Shader> leaf_shaders;
        leaf_shaders.emplace_back("tree.vert");
        leaf_shaders.emplace_back("tree_leaf.frag");
        ShaderProgram leaf_shader_prog(leaf_shaders);

        Texture2D tree_trunk = Texture2D("./tree/bark_0021.jpg");  //
        Texture2D tree_leaf = Texture2D("./tree/DB2X2_L01.png");   //
        Texture2D tree_leaf_normal_map = Texture2D("./tree/DB2X2_L01_Nor.png");   //
        Texture2D tree_leaf_specular_map = Texture2D("./tree/DB2X2_L01_Spec.png");   //

        // ʹ�����ɵ���ɫ������
        trunk_shader_prog.use();
        // �����ɵ�����
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
        tree_trunk.bind();
        trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
        
        // ʹ����Ҷ����ɫ������  
        leaf_shader_prog.use();
        //��Ҷ��ͼ
        // ����Ҷ�Ļ�����ɫ��ͼ        
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
        tree_leaf.bind();
        leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
        // ����Ҷ�ķ�����ͼ
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
        tree_leaf_normal_map.bind();  // �����Ѽ���Ϊ tree_normal_map
        leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
        // ����Ҷ���ض�������ͼ
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
        tree_leaf_specular_map.bind();  // �����Ѽ���Ϊ tree_specular_map
        leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);

        
        //*/
        //-----------------------//
        // ������2��ģ��
        // ���� tree8.obj �ļ�
        ObjMesh tree2Mesh = load_wavefront_obj(BASE_PATH + "tree8.obj");

        Texture2D tree2_bark = Texture2D("./tree2/bark.png");  //
        Texture2D tree2_leaf = Texture2D("./tree2/leaf.png");   //
        Texture2D tree2_leaf_normal_map = Texture2D("./tree2/leaf_normal.png");   //
        Texture2D tree2_leaf_specular_map = Texture2D("./tree2/leaf_specular.png");   //

        // ʹ�����ɵ���ɫ������
        trunk_shader_prog.use();
        // �����ɵ�����
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
        tree2_bark.bind();
        trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);

        // ʹ����Ҷ����ɫ����
        leaf_shader_prog.use();
        // ����Ҷ������
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
        tree2_leaf.bind();
        leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
        // ����Ҷ�ķ�����ͼ
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
        tree2_leaf_normal_map.bind();  // �����Ѽ���Ϊ tree_normal_map
        leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
        // ����Ҷ���ض�������ͼ
        glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
        tree2_leaf_specular_map.bind();  // �����Ѽ���Ϊ tree_specular_map
        leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);


        //*/
        // ����һȺʯͷģ��
        // ���� rocks.obj �ļ�
        ObjMesh rocksMesh = load_wavefront_obj(BASE_PATH + "rocks.obj");

        std::vector<Shader> rocks_shaders;
        rocks_shaders.emplace_back("rocks.vert");
        rocks_shaders.emplace_back("rocks.frag");
        ShaderProgram rocks_shader_prog(rocks_shaders);

        Texture2D rocks_texture = Texture2D("./rocks/Handle0.jpg");  //

        //ʹ����ʯ����ɫ������
        rocks_shader_prog.use();

        // ����ʯ������
        glActiveTexture(GL_TEXTURE0 + 21);
        rocks_texture.bind();
        rocksMesh.renderPart("AssortedRocks"); // ��Ⱦ������glass����
        //*/

        // ���ش�ʯͷģ��
        // ���� caverock.obj �ļ�
        ObjMesh caverockMesh = load_wavefront_obj(BASE_PATH + "caverock.obj");

        std::vector<Shader> caverock_shaders;
        caverock_shaders.emplace_back("caverock.vert");
        caverock_shaders.emplace_back("caverock.frag");
        ShaderProgram caverock_shader_prog(caverock_shaders);

        Texture2D caverock_texture = Texture2D("./rocks/Ground.jpg");  //

        //ʹ�ô�ʯͷ����ɫ������
        caverock_shader_prog.use();

        // �󶨴�ʯͷ������
        glActiveTexture(GL_TEXTURE0 + 23);
        caverock_texture.bind();
        caverockMesh.renderPart("CavePlatform4"); // ��Ⱦ������glass����
        //*/


        // ������ͨʯͷģ��
        // ���� SmallArch_Obj.obj �ļ�
        ObjMesh stoneMesh = load_wavefront_obj(BASE_PATH + "SmallArch_Obj.obj");
        //ObjMesh stoneMesh = load_wavefront_obj(BASE_PATH + "CaveWalls4_B.obj");

        std::vector<Shader> stone_shaders;
        stone_shaders.emplace_back("stone.vert");
        stone_shaders.emplace_back("stone.frag");
        ShaderProgram stone_shader_prog(stone_shaders);

        Texture2D stone_texture = Texture2D("./stone/DSC_4736.jpg");  //

        //ʹ����ͨʯͷ����ɫ������
        stone_shader_prog.use();

        // ����ͨʯͷ������
        glActiveTexture(GL_TEXTURE0 + 1);
        stone_texture.bind();
        stoneMesh.render();
        //*/

        // ������ͨʯͷ2ģ��
        // ���� CaveWalls4_B.obj �ļ�
        ObjMesh stone2Mesh = load_wavefront_obj(BASE_PATH + "CaveWalls4_B.obj");

        std::vector<Shader> stone2_shaders;
        stone2_shaders.emplace_back("stone.vert");
        stone2_shaders.emplace_back("stone.frag");
        ShaderProgram stone2_shader_prog(stone_shaders);

        Texture2D stone2_texture = Texture2D("./rocks/window.jpg");  //

        //ʹ����ͨʯͷ����ɫ������
        stone2_shader_prog.use();

        // ����ͨʯͷ������
        glActiveTexture(GL_TEXTURE0 + 25);
        stone2_texture.bind();
        stone2Mesh.render();
        //*/

        //��������������������������������������������������������//

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

            //// --- Point Light Parameters
            //glm::vec3 pLightColour = m_context.m_gui_param.plight_color;
            //float pLightStrength = m_context.m_gui_param.plight_strength;

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
                    // ��Ⱦѭ�������� uniform ����
                    ocean_shader_prog_fresnel.use();
                    ocean_shader_prog_fresnel.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_fresnel.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_fresnel.setFloat("light.strength", dLightStrength);

                    ocean_renderer_fresnel.render(m_context.m_render_camera);
                }
                    break;
                case 1: {
                    // ��Ⱦѭ�������� uniform ����
                    ocean_shader_prog_refl.use();
                    ocean_shader_prog_refl.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_refl.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_refl.setFloat("light.strength", dLightStrength);

                    ocean_renderer_refl.render(m_context.m_render_camera);
                }
                    break;
                case 2: {
                    // ��Ⱦѭ�������� uniform ����
                    ocean_shader_prog_refr.use();
                    ocean_shader_prog_refr.setVec3("light.direction", dLightDirection);
                    ocean_shader_prog_refr.setVec3("light.colour", dLightColour);
                    ocean_shader_prog_refr.setFloat("light.strength", dLightStrength);
                    
                    ocean_renderer_refr.render(m_context.m_render_camera);
                }
                    break;
                default:
                {
                    // ��Ⱦѭ�������� uniform ����
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
            // ��Ⱦѭ�������� uniform ����
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
                        m_context.m_gui_param.raindrop_max_speed);
                }

                rain.computeRainOnGPU(ImGui::GetIO().DeltaTime,
                    m_context.m_gui_param.rain_sea_level,
                    m_context.m_gui_param.rain_position,
                    m_context.m_gui_param.rain_radius,
                    m_context.m_gui_param.raindrop_min_speed,
                    m_context.m_gui_param.raindrop_max_speed);
                rain.renderSplashes(proj, view, cameraRight, cameraUp, ImGui::GetIO().DeltaTime);
            }

            //��������������������������������������������������������������������//
            //��ȾOBJ�ļ�
            //glm::vec3 light_pos(
            //    m_context.m_gui_param.light_pos_x,
            //    m_context.m_gui_param.light_pos_y,
            //    m_context.m_gui_param.light_pos_z
            //);

            //glm::vec3 light_color(
            //    m_context.m_gui_param.light_color_x,
            //    m_context.m_gui_param.light_color_y,
            //    m_context.m_gui_param.light_color_z
            //);

            // ��Ⱦ����ģ��
            lighthouse_shader_prog.use();  // ʹ�õ���ģ�͵���ɫ������
            //lighthouse_shader_prog.setVec3("light_pos", light_pos);    // ���ù���λ��
            //lighthouse_shader_prog.setVec3("light_color", light_color);  // ���ù�Դ��ɫ

            lighthouse_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            lighthouse_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ

            lighthouse_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            lighthouse_shader_prog.setInt("normalMap", 0); // 0��ʾ�󶨵�������Ԫ

            // �趨��ɫ������ɫ
            lighthouse_shader_prog.setVec3("object_color", glm::vec3(0.5f, 0.5f, 0.5f));

            // ����ģ�;���
            glm::mat4 lighthouse_model_matrix = glm::translate(glm::mat4(0.3f), glm::vec3(-80.0f, 15.0f, -420.0f)); // ƽ�Ʊ任
            lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            lighthouse_model_matrix = glm::rotate(lighthouse_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // ��Z��˳ʱ����ת90��
            lighthouse_shader_prog.setMat4("model", lighthouse_model_matrix);

            //glm::mat4 lighthouse_model_matrix = glm::translate(glm::mat4(0.2f), glm::vec3(-20.0f, 0.0f, -160.0f));  // ģ�ͱ任
            //lighthouse_shader_prog.setMat4("model", lighthouse_model_matrix);

            // ������ͼ��ͶӰ����
            lighthouse_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            lighthouse_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // ��Ⱦ����ģ��
            //lighthouseMesh.render();
            //lighthouse_shader_prog.setInt("texture1", 0);  // ������ͼ��Ԫ

            glActiveTexture(GL_TEXTURE0 + 5);
            lighthouse_iron.bind();
            lighthouse_shader_prog.setInt("texture1", 5);
            lighthouseMesh.renderPart("Bl_iron");

            // ��Ⱦ bl_glass ����
            glActiveTexture(GL_TEXTURE0 + 6);
            lighthouse_blglass.bind();
            lighthouse_shader_prog.setInt("texture1", 6);
            lighthouseMesh.renderPart("bl_glass");

            // ��Ⱦ clglass ����
            glActiveTexture(GL_TEXTURE0 + 7);
            lighthouse_glass.bind();
            lighthouse_shader_prog.setInt("texture1", 7);
            lighthouseMesh.renderPart("clglass");

            // ��Ⱦ lens ����
            glActiveTexture(GL_TEXTURE0 + 8);
            lighthouse_lens.bind();
            lighthouse_shader_prog.setInt("texture1", 8);
            lighthouseMesh.renderPart("lens");

            // ��Ⱦ red_iron ����
            glActiveTexture(GL_TEXTURE0 + 10);
            lighthouse_rediron.bind();
            lighthouse_shader_prog.setInt("texture1", 10);
            lighthouseMesh.renderPart("red_iron");

            // ��Ⱦ rock ����
            glActiveTexture(GL_TEXTURE0 + 11);
            lighthouse_rock.bind();
            lighthouse_shader_prog.setInt("texture1", 11);
            lighthouseMesh.renderPart("rock");

            // ��Ⱦ walls ����
            glActiveTexture(GL_TEXTURE0 + 12);
            lighthouse_wall.bind();
            lighthouse_shader_prog.setInt("texture1", 12);
            lighthouseMesh.renderPart("walls");

            // ��Ⱦ wood ����
            glActiveTexture(GL_TEXTURE0 + 13);
            lighthouse_wood.bind();
            lighthouse_shader_prog.setInt("texture1", 13);
            lighthouseMesh.renderPart("wood");


            //-----------------------------//
            // ��Ⱦ��1ģ��
            trunk_shader_prog.use();
            trunk_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            trunk_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ
            trunk_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            // ����ģ�;���
            glm::mat4 tree_model_matrix = glm::translate(glm::mat4(5.0f), glm::vec3(-5.0f, -1.1f, -12.0f));  // ģ�ͱ任
            tree_model_matrix = glm::scale(tree_model_matrix, glm::vec3(0.6f, 0.6f, 0.6f));
            trunk_shader_prog.setMat4("model", tree_model_matrix);
            // ������ͼ��ͶӰ����
            trunk_shader_prog.setMat4("view", view);
            trunk_shader_prog.setMat4("projection", proj);
            // ��Ⱦ ���� ����
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
            tree_trunk.bind();
            trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_TRUNK_DIFFUSE);
            treeMesh.renderPart("bark");

            // ��Ⱦ ��Ҷ ����
            leaf_shader_prog.use();
            leaf_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            leaf_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ
            leaf_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            // ����ģ�;���
            leaf_shader_prog.setMat4("model", tree_model_matrix);
            // ������ͼ��ͶӰ����
            leaf_shader_prog.setMat4("view", view);
            leaf_shader_prog.setMat4("projection", proj);
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
            tree_leaf.bind();
            leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_DIFFUSE);
            // ����Ҷ�ķ�����ͼ
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
            tree_leaf_normal_map.bind();  // �����Ѽ���Ϊ tree_normal_map
            leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_NORMAL);
            // ����Ҷ���ض�������ͼ
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
            tree_leaf_specular_map.bind();  // �����Ѽ���Ϊ tree_specular_map
            leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE1_LEAF_SPECULAR);
            treeMesh.renderPart("leaf");
            //*/


            //-----------------------------//
            // ��Ⱦ��2ģ��
            trunk_shader_prog.use();
            trunk_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            trunk_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ
            trunk_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            // ����ģ�;���
            glm::mat4 tree2_model_matrix = glm::translate(glm::mat4(1.5f), glm::vec3(-12.0f, -3.5f, -25.0f));  // ģ�ͱ任
            trunk_shader_prog.setMat4("model", tree2_model_matrix);
            // ������ͼ��ͶӰ����
            trunk_shader_prog.setMat4("view", view);
            trunk_shader_prog.setMat4("projection", proj);
            // ��Ⱦ ���� ����
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
            tree2_bark.bind();
            trunk_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_TRUNK_DIFFUSE);
            tree2Mesh.renderPart("bark");

            // ʹ����Ҷ��ɫ����
            leaf_shader_prog.use();
            leaf_shader_prog.setVec3("light_dir", dLightDirection);     // ���ù���λ��
            leaf_shader_prog.setVec3("light_color", dLightColour);      // ���ù�Դ��ɫ
            leaf_shader_prog.setVec3("camera_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            // ������ͼ��ͶӰ����
            leaf_shader_prog.setMat4("model", tree2_model_matrix);
            leaf_shader_prog.setMat4("view", view);
            leaf_shader_prog.setMat4("projection", proj);
            // ����Ҷ������
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
            tree2_leaf.bind();
            leaf_shader_prog.setInt("texture1", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_DIFFUSE);
            // ����Ҷ�ķ�����ͼ
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
            tree2_leaf_normal_map.bind();  // �����Ѽ���Ϊ tree_normal_map
            leaf_shader_prog.setInt("normalMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_NORMAL);
            // ����Ҷ���ض�������ͼ
            glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
            tree2_leaf_specular_map.bind();  // �����Ѽ���Ϊ tree_specular_map
            leaf_shader_prog.setInt("specularMap", CGRA350Constants::TEX_SAMPLE_ID_TREE2_LEAF_SPECULAR);
            tree2Mesh.renderPart("leaf");
            //*/


            //-----------------------------//
            // ��Ⱦʯͷģ��
            rocks_shader_prog.use();  // ʹ�õ���ģ�͵���ɫ������
            //rocks_shader_prog.setVec3("light_pos", light_pos);    // ���ù���λ��
            //rocks_shader_prog.setVec3("light_color", light_color);  // ���ù�Դ��ɫ

            rocks_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            rocks_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ

            rocks_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            rocks_shader_prog.setInt("normalMap", 0); // 0��ʾ�󶨵�������Ԫ

            // �趨ʯͷ������ɫ
            rocks_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // ����ģ�;���
            glm::mat4 rocks_model_matrix = glm::translate(glm::mat4(0.9f), glm::vec3(-280.0f, -37.0f, -180.0f));  // ģ�ͱ任
            rocks_model_matrix = glm::rotate(rocks_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            rocks_shader_prog.setMat4("model", rocks_model_matrix);

            // ������ͼ��ͶӰ����
            rocks_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            rocks_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // ��Ⱦ ʯͷ ����
            glActiveTexture(GL_TEXTURE0 + 21);
            rocks_texture.bind();
            rocks_shader_prog.setInt("texture1", 21);
            rocksMesh.renderPart("AssortedRocks");
            //*/


            //-----------------------------//
            // ��Ⱦ��ʯͷģ��
            caverock_shader_prog.use();  // ʹ�ô�ʯͷģ�͵���ɫ������
            //caverock_shader_prog.setVec3("light_pos", light_pos);    // ���ù���λ��
            //caverock_shader_prog.setVec3("light_color", light_color);  // ���ù�Դ��ɫ

            caverock_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            caverock_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ

            caverock_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            caverock_shader_prog.setInt("normalMap", 0); // 0��ʾ�󶨵�������Ԫ

            // �趨��ʯͷ������ɫ
            caverock_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // ����ģ�;���
            glm::mat4 caverock_model_matrix = glm::translate(glm::mat4(0.2f), glm::vec3(-120.0f, -10.0f, -1090.0f));  // ģ�ͱ任
            caverock_model_matrix = glm::rotate(caverock_model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            caverock_shader_prog.setMat4("model", caverock_model_matrix);

            // ������ͼ��ͶӰ����
            caverock_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            caverock_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // ��Ⱦ ��ʯͷ ����
            glActiveTexture(GL_TEXTURE0 + 23);
            caverock_texture.bind();
            caverock_shader_prog.setInt("texture1", 23);
            caverockMesh.renderPart("CavePlatform4");
            //*/

            //-----------------------------//
            // ��Ⱦ��ͨʯͷģ��
            stone_shader_prog.use();  // ʹ����ͨʯͷģ�͵���ɫ������
            //stone_shader_prog.setVec3("light_pos", light_pos);    // ���ù���λ��
            //stone_shader_prog.setVec3("light_color", light_color);  // ���ù�Դ��ɫ

            stone_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            stone_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ

            stone_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            stone_shader_prog.setInt("normalMap", 0); // 0��ʾ�󶨵�������Ԫ

            // �趨��ͨʯͷ������ɫ
            stone_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // ����ģ�;���
            glm::mat4 stone_model_matrix = glm::translate(glm::mat4(0.2f), glm::vec3(-340.0f, -25.0f, -180.0f));  // ģ�ͱ任
            stone_model_matrix = glm::rotate(stone_model_matrix, glm::radians(-100.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            stone_shader_prog.setMat4("model", stone_model_matrix);

            // ������ͼ��ͶӰ����
            stone_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            stone_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // ��Ⱦ ��ͨʯͷ ����
            glActiveTexture(GL_TEXTURE0 + 1);
            stone_texture.bind();
            stone_shader_prog.setInt("texture1", 1);
            stoneMesh.render();
            //*/


            //-----------------------------//
            // ��Ⱦ��ͨʯͷ2ģ��
            stone2_shader_prog.use();  // ʹ����ͨʯͷģ�͵���ɫ������
            //stone2_shader_prog.setVec3("light_pos", light_pos);    // ���ù���λ��
            //stone2_shader_prog.setVec3("light_color", light_color);  // ���ù�Դ��ɫ

            stone2_shader_prog.setVec3("light_dir", dLightDirection);    // ���ù���λ��
            stone2_shader_prog.setVec3("light_color", dLightColour);  // ���ù�Դ��ɫ

            stone2_shader_prog.setVec3("view_pos", m_context.m_render_camera.getPosition()); // ���������λ��
            stone2_shader_prog.setInt("normalMap", 0); // 0��ʾ�󶨵�������Ԫ

            // �趨��ͨʯͷ������ɫ
            stone2_shader_prog.setVec3("object_color", glm::vec3(0.5f, 2.5f, 0.5f));

            // ����ģ�;���
            glm::mat4 stone2_model_matrix = glm::translate(glm::mat4(0.6f), glm::vec3(-35.0f, -5.0f, -30.0f));  // ģ�ͱ任
            stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // ��Z��˳ʱ����ת90��
            stone2_model_matrix = glm::rotate(stone2_model_matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // ��Z��˳ʱ����ת90��
            stone2_shader_prog.setMat4("model", stone2_model_matrix);

            // ������ͼ��ͶӰ����
            stone2_shader_prog.setMat4("view", m_context.m_render_camera.getViewMatrix());
            stone2_shader_prog.setMat4("projection", m_context.m_render_camera.getProjMatrix());

            // ��Ⱦ ��ͨʯͷ ����
            glActiveTexture(GL_TEXTURE0 + 25);
            stone2_texture.bind();
            stone2_shader_prog.setInt("texture1", 25);
            stone2Mesh.render();
            //*/
            //��������������������������������������������������������������������//

            // --- render cloud ---
            if (m_context.m_do_render_cloud)
            {
                RenderCloud(m_context.m_render_camera, *m_volumerender, m_window.getWindow());
            }

            // --- render Rain Drops ---
            if (m_context.m_do_render_rain)
            {
                rain.renderRaindrops(proj, view, ImGui::GetIO().DeltaTime,
                    m_context.m_gui_param.raindrop_length,
                    m_context.m_gui_param.raindrop_color);
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
