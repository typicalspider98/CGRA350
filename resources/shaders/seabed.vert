#version 430 core

#define PI 3.14159265358979323846

layout (location = 0) in vec3 oc_pos;  // ����λ��
layout (location = 1) in vec2 tex_coords;  // ��������

out VS_OUT
{
    vec3 wc_pos;    // ��������λ��
    vec3 wc_normal; // ����
    vec2 tex_coords; // ��������
} vs_out;

uniform mat4 m_matrix;  // ģ�;���
uniform mat4 vp_matrix; // ��ͼ-ͶӰ����
uniform sampler2D perlin_tex; // Perlin ��������

void main()
{
    // ����������ת����������
    vec4 wc_pos = m_matrix * vec4(oc_pos, 1.0);

    // �� Perlin ���������л�ȡλ��ֵ
    float displ = texture(perlin_tex, tex_coords).r;

    // ���ﱣ�� Perlin ����Ӱ�죬������ x �����б��λ��
    float slope = (wc_pos.x + 145) * 0.35;  // б����ƣ��¶�Ϊ 0.3��������ֵ�Կ���б����б��

    // �ж��Ƿ����Ұ벿��
    if ((wc_pos.x > -145.0)&&(wc_pos.x < -60.0)) {  // ���� x > 0 �Ĳ���Ϊ�Ұ벿�֣�����Ը���ʵ����Ҫ����
        wc_pos.y += slope;  // ֻ���Ұ벿��Ӧ��б��Ч��
    }

    if(wc_pos.x > -60.0){
        wc_pos.y += 29.75;
    }

    // ���� Perlin ����Ӱ��
    wc_pos.y += 20 * displ;  // 20 ��ԭʼ�� Perlin �����Ŵ�ϵ����40 ����������

    vs_out.wc_pos = vec3(wc_pos);

    // ���㷨��
    float delta = 0.001;
    vec3 wc_pos_displaced_dx = vec3(1, 20*(texture(perlin_tex, tex_coords + vec2(delta, 0)).r - displ), 0);
    vec3 wc_pos_displaced_dz = vec3(0, 20*(texture(perlin_tex, tex_coords + vec2(0, delta)).r - displ), 1);
    vs_out.wc_normal = normalize(cross(wc_pos_displaced_dx, wc_pos_displaced_dz));

    // �����������
    vs_out.tex_coords = tex_coords;

    // �������յ���Ļ�ռ�����
    gl_Position = vp_matrix * wc_pos;
}