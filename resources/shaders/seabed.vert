#version 430 core

#define PI 3.14159265358979323846

layout (location = 0) in vec3 oc_pos;  // 顶点位置
layout (location = 1) in vec2 tex_coords;  // 纹理坐标

out VS_OUT
{
    vec3 wc_pos;    // 世界坐标位置
    vec3 wc_normal; // 法线
    vec2 tex_coords; // 纹理坐标
} vs_out;

uniform mat4 m_matrix;  // 模型矩阵
uniform mat4 vp_matrix; // 视图-投影矩阵
uniform sampler2D perlin_tex; // Perlin 噪声纹理

void main()
{
    // 将物体坐标转到世界坐标
    vec4 wc_pos = m_matrix * vec4(oc_pos, 1.0);

    // 从 Perlin 噪声纹理中获取位移值
    float displ = texture(perlin_tex, tex_coords).r;

    // 这里保留 Perlin 噪声影响，并增加 x 方向的斜面位移
    float slope = (wc_pos.x + 145) * 0.35;  // 斜面控制，坡度为 0.3，调整此值以控制斜面倾斜度

    // 判断是否在右半部分
    if ((wc_pos.x > -145.0)&&(wc_pos.x < -60.0)) {  // 假设 x > 0 的部分为右半部分，你可以根据实际需要调整
        wc_pos.y += slope;  // 只对右半部分应用斜面效果
    }

    if(wc_pos.x > -60.0){
        wc_pos.y += 29.75;
    }

    // 保留 Perlin 噪声影响
    wc_pos.y += 20 * displ;  // 20 是原始的 Perlin 噪声放大系数，40 是整体提升

    vs_out.wc_pos = vec3(wc_pos);

    // 计算法线
    float delta = 0.001;
    vec3 wc_pos_displaced_dx = vec3(1, 20*(texture(perlin_tex, tex_coords + vec2(delta, 0)).r - displ), 0);
    vec3 wc_pos_displaced_dz = vec3(0, 20*(texture(perlin_tex, tex_coords + vec2(0, delta)).r - displ), 1);
    vs_out.wc_normal = normalize(cross(wc_pos_displaced_dx, wc_pos_displaced_dz));

    // 输出纹理坐标
    vs_out.tex_coords = tex_coords;

    // 计算最终的屏幕空间坐标
    gl_Position = vp_matrix * wc_pos;
}