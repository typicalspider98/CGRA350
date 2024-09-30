#include "volume_gui.hpp"

using namespace std;

HWND GL_Window;
HWND DX_Window;
int actual_frame = 0;
GUIs* gui = NULL;
float3* accum_buffer = NULL;
Histogram* histo_buffer_cuda = NULL;
cudaGraphicsResource_t display_buffer_cuda = NULL;
GLuint tempBuffer = 0;
GLuint tempTex = 0;
GLuint display_buffer = 0;
GLuint display_tex = 0;
GLuint program = 0;
GLuint quad_vertex_buffer = 0;
GLuint quad_vao = 0;

#define CheckError { auto error = cudaGetLastError(); if (error != 0) cout << cudaGetErrorString(error) << endl; }

static void init_cuda()
{
    int cuda_devices[1];
    unsigned int num_cuda_devices;
    cudaGLGetDevices(&num_cuda_devices, cuda_devices, 1, cudaGLDeviceListAll);
    if (num_cuda_devices == 0) {
        fprintf(stderr, "Could not determine CUDA device for current OpenGL context\n.");
        exit(EXIT_FAILURE);
    }
    cudaSetDevice(cuda_devices[0]);
}

#pragma region Shader
static GLint add_shader(GLenum shader_type, const char* source_code, GLuint program)
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source_code, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    glAttachShader(program, shader);

    return shader;
}

static GLuint create_shader_program()
{
    GLint success;
    GLuint program = glCreateProgram();

    const char* vert =
        "#version 330\n"
        "in vec3 Position;"
        "out vec2 TexCoord;"
        "void main() {"
        "    gl_Position = vec4(Position, 1.0);"
        "    TexCoord = 0.5 * Position.xy + vec2(0.5);"
        "}";
    add_shader(GL_VERTEX_SHADER, vert, program);

    const char* frag =
        "#version 330\n"
        "in vec2 TexCoord;"
        "out vec4 FragColor;"
        "uniform sampler2D TexSampler;"
        "uniform int Size;"
        "uniform int FSR;"
        "uniform float sharp;"
        "void FsrEASU(out vec4 fragColor){    vec2 fragCoord = TexCoord * Size * 2;    vec4 scale = vec4(        1. / vec2(Size),        vec2(0.5f)    );    vec2 src_pos = scale.zw * fragCoord;    vec2 src_centre = floor(src_pos - .5) + .5;    vec4 f; f.zw = 1. - (f.xy = src_pos - src_centre);    vec4 l2_w0_o3 = ((1.5672 * f - 2.6445) * f + 0.0837) * f + 0.9976;    vec4 l2_w1_o3 = ((-0.7389 * f + 1.3652) * f - 0.6295) * f - 0.0004;    vec4 w1_2 = l2_w0_o3;    vec2 w12 = w1_2.xy + w1_2.zw;    vec4 wedge = l2_w1_o3.xyzw * w12.yxyx;    vec2 tc12 = scale.xy * (src_centre + w1_2.zw / w12);    vec2 tc0 = scale.xy * (src_centre - 1.);    vec2 tc3 = scale.xy * (src_centre + 2.);    vec3 col = vec3(        texture(TexSampler, vec2(tc12.x, tc0.y)).rgb * wedge.y +        texture(TexSampler, vec2(tc0.x, tc12.y)).rgb * wedge.x +        texture(TexSampler, tc12.xy).rgb * (w12.x * w12.y) +        texture(TexSampler, vec2(tc3.x, tc12.y)).rgb * wedge.z +        texture(TexSampler, vec2(tc12.x, tc3.y)).rgb * wedge.w    );    fragColor = vec4(col,1);}void FsrRCAS(float sharp, out vec4 fragColor){    vec2 uv = TexCoord;    vec3 col = texture(TexSampler, uv).xyz;    float max_g = col.y;    float min_g = col.y;    vec4 uvoff = vec4(1,0,1,-1)/Size;    vec3 colw;    vec3 col1 = texture(TexSampler, uv+uvoff.yw).xyz;    max_g = max(max_g, col1.y);    min_g = min(min_g, col1.y);    colw = col1;    col1 = texture(TexSampler, uv+uvoff.xy).xyz;    max_g = max(max_g, col1.y);    min_g = min(min_g, col1.y);    colw += col1;    col1 = texture(TexSampler, uv+uvoff.yz).xyz;    max_g = max(max_g, col1.y);    min_g = min(min_g, col1.y);    colw += col1;    col1 = texture(TexSampler, uv-uvoff.xy).xyz;    max_g = max(max_g, col1.y);    min_g = min(min_g, col1.y);    colw += col1;    float d_min_g = min_g;    float d_max_g = 1.-max_g;    float A;    max_g = max(0., max_g);    if (d_max_g < d_min_g) {        A = d_max_g / max_g;    } else {        A = d_min_g / max_g;    }    A = sqrt(max(0., A));    A *= mix(-.125, -.2, sharp);    vec3 col_out = (col + colw * A) / (1.+4.*A);    fragColor = vec4(col_out,1);}"
        "void main() {"
        "    if (FSR == 1) FsrEASU(FragColor);"
        "    else if (FSR == 2) FsrRCAS(sharp, FragColor);"
        "    else FragColor = texture(TexSampler, TexCoord);"
        "}";
    GLint fs = add_shader(GL_FRAGMENT_SHADER, frag, program);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        fprintf(stderr, "Error linking shadering program\n");
        char info[10240];
        int len;
        glGetShaderInfoLog(fs, 10240, &len, info);
        fprintf(stderr, info);
        glfwTerminate();
    }

    glUseProgram(program);

    return program;
}

// Create a quad filling the whole screen.
static GLuint create_quad(GLuint program, GLuint* vertex_buffer)
{
    static const float3 vertices[6] = {
        { -1.f, -1.f, 0.0f },
        {  1.f, -1.f, 0.0f },
        { -1.f,  1.f, 0.0f },
        {  1.f, -1.f, 0.0f },
        {  1.f,  1.f, 0.0f },
        { -1.f,  1.f, 0.0f }
    };

    glGenBuffers(1, vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    const GLint pos_index = glGetAttribLocation(program, "Position");
    glEnableVertexAttribArray(pos_index);
    glVertexAttribPointer(
        pos_index, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

    return vertex_array;
}

static void resize_buffers(float3** accum_buffer_cuda, Histogram** histo_buffer_cuda, cudaGraphicsResource_t* display_buffer_cuda, GLuint tempFB, GLuint* tempTex, int width, int width2, GLuint display_buffer)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display_buffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * width * 4, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    if (*display_buffer_cuda)
        cudaGraphicsUnregisterResource(*display_buffer_cuda);
    cudaGraphicsGLRegisterBuffer(
        display_buffer_cuda, display_buffer, cudaGraphicsRegisterFlagsWriteDiscard);

    if (*accum_buffer_cuda)
        cudaFree(*accum_buffer_cuda);
    cudaMalloc(accum_buffer_cuda, width * width * sizeof(float3));

    if (*histo_buffer_cuda)
        cudaFree(*histo_buffer_cuda);
    cudaMalloc(histo_buffer_cuda, width * width * sizeof(Histogram));

    glDeleteTextures(1, tempTex);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFB);
    glGenTextures(1, tempTex);
    glBindTexture(GL_TEXTURE_2D, *tempTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, width2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *tempTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
#pragma endregion


void CheckCuda()
{
    cudaError_t err;
    int deviceCount = 0;
    err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        std::cerr << "CUDA Error: No CUDA devices found: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    std::cout << "Number of CUDA devices: " << deviceCount << std::endl;

    int device;
    err = cudaGetDevice(&device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to get current device: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    cudaDeviceProp deviceProp;
    err = cudaGetDeviceProperties(&deviceProp, device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to get device properties: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    std::cout << "Using device: " << deviceProp.name << std::endl;

    err = cudaSetDevice(device);
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: Unable to set device: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    size_t freeMem, totalMem;
    cudaMemGetInfo(&freeMem, &totalMem);
    std::cout << "Free memory: " << freeMem << " bytes" << std::endl;
    std::cout << "Total memory: " << totalMem << " bytes" << std::endl;
    CheckError
        cudaFree(0);
    CheckError;
}

void InitVR(Camera_VR& cam, VolumeRender& volume, float3 lightDir, float3 lightColor, float3 scatter_rate, float alpha, float multiScatterNum, float g) {
#ifdef GUI
    gui = new GUIs();

    gui->G = g;
    gui->alpha = alpha;
    gui->ms = multiScatterNum;
    gui->lighta = atan2(lightDir.z, lightDir.x);
    gui->lighty = atan2(lightDir.y, sqrt(max(0.0001f, lightDir.x * lightDir.x + lightDir.z * lightDir.z)));
    gui->lightColor = lightColor;
    gui->scatter_rate = scatter_rate;

    volume.UpdateHGLut(g);

    glGenBuffers(1, &display_buffer);
    glGenTextures(1, &display_tex);

    glGenFramebuffers(1, &tempBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, tempBuffer);
    glGenTextures(1, &tempTex);

    glBindTexture(GL_TEXTURE_2D, tempTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cam.resolution, cam.resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    program = create_shader_program();
    quad_vao = create_quad(program, &quad_vertex_buffer);

    init_cuda();
#endif
}

void RenderVR(Camera_VR& cam, VolumeRender& volume, GLFWwindow* window)
{
    if (gui == NULL)
        return;

    // Reallocate buffers if window size changed.
    int nwidth, nheight;
    glfwGetFramebufferSize(window, &nwidth, &nheight);
    if (gui->fsr) {
        nwidth /= 2; nheight /= 2;
    }
    if (nwidth != gui->width || nheight != gui->height)
    {
        gui->width = nwidth;
        gui->height = nheight;

        resize_buffers(
            &accum_buffer, &histo_buffer_cuda, &display_buffer_cuda, tempBuffer, &tempTex, gui->width, gui->fsr ? gui->width * 2 : gui->width, display_buffer);
        //kernel_params.accum_buffer = accum_buffer;

        if (gui->fsr)
            glViewport(0, 0, gui->width * 2, gui->height * 2);
        else
            glViewport(0, 0, gui->width, gui->height);

        gui->frame = 0;

        //kernel_params.resolution.x = width;
        //kernel_params.resolution.y = height;
        //kernel_params.iteration = 0;

        // Allocate texture once
        glBindTexture(GL_TEXTURE_2D, display_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gui->width, gui->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Map GL buffer for access with CUDA.
    cudaGraphicsMapResources(1, &display_buffer_cuda, /*stream=*/0);
    void* p;
    size_t size_p;
    cudaGraphicsResourceGetMappedPointer(&p, &size_p, display_buffer_cuda);

    if (!gui->pause) {
        volume.SetEnvExp(gui->env_exp);
        volume.SetTrScale(gui->tr);
        volume.SetScatterRate(gui->scatter_rate * 1.001);
        volume.SetExposure(gui->exposure);
        volume.SetSurfaceIOR(gui->render_surface ? gui->IOR : -1);
        volume.SetCheckboard(gui->checkboard);

        if (gui->change_hdri) {
            volume.SetHDRI(gui->hdri_path);
            gui->change_hdri = false;
            gui->frame = 0;
        }

        float3 l;
        float altiangle = gui->lighty;
        l.y = sin(altiangle);
        float aziangle = gui->lighta;
        l.x = cos(aziangle) * cos(altiangle);
        l.z = sin(aziangle) * cos(altiangle);

        auto start_time = std::chrono::system_clock::now();

        cam.Render(accum_buffer, histo_buffer_cuda, reinterpret_cast<unsigned int*>(p), int2{ gui->width , gui->height }, gui->frame, l, gui->lightColor, gui->alpha, gui->ms, gui->G, gui->toneType,
                   gui->predict ? (gui->mrpnn ? VolumeRender::RenderType::MRPNN : VolumeRender::RenderType::RPNN) : VolumeRender::RenderType::PT,
                   gui->denoise);

        auto finish_time = std::chrono::system_clock::now();
        float new_fps = 10000000.0f / (finish_time - start_time).count();
        gui->fps = lerp(gui->fps, new_fps, abs(new_fps - gui->fps) / gui->fps > 0.3 ? 1.0f : 0.01f);

        gui->frame++;
        actual_frame++;
    }

    // Unmap GL buffer.
    cudaGraphicsUnmapResources(1, &display_buffer_cuda, /*stream=*/0);

    // Update texture for display.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display_buffer);
    glBindTexture(GL_TEXTURE_2D, display_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gui->width, gui->height, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Render the quad.
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quad_vao);

    glUniform1i(glGetUniformLocation(program, "Size"), gui->width);
    if (gui->fsr) {
        glUniform1f(glGetUniformLocation(program, "sharp"), gui->sharpness);
        glBindFramebuffer(GL_FRAMEBUFFER, tempBuffer);
        glUniform1i(glGetUniformLocation(program, "FSR"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, tempTex);
        glUniform1i(glGetUniformLocation(program, "FSR"), 2);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUniform1i(glGetUniformLocation(program, "FSR"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void CleanupVR()
{
    cudaFree(accum_buffer);

    // Cleanup OpenGL.
    glDeleteVertexArrays(1, &quad_vao);
    glDeleteBuffers(1, &quad_vertex_buffer);
    glDeleteProgram(program);
}

