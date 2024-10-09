#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // ���������
uniform vec3 object_color;          // ������ɫ
uniform vec3 view_pos;              // �����λ��
uniform float roughness;            // �ֲڶ�
uniform float metalness;            // ������
uniform float reflectivity;         // ������

// ���� DirectionalLight �ṹ��
struct DirectionalLight {
    vec3 colour;
    vec3 direction;
    float strength;
};

// �� light ����Ϊ uniform
uniform DirectionalLight light;

// �̶��Ļ��������
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // ��ɫ������
const float ambient_strength = 0.2;                    // ������ǿ��

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // ���ߡ���Դ��������߷���ĵ�λ��
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction); // ʹ�� DirectionalLight �еķ���
    vec3 V = normalize(view_pos - FragPos);
    vec3 H = normalize(V + L);

    // ���������������
    vec3 F0 = mix(vec3(0.04), object_color, metalness);  // ʹ�û��ڽ����ȵĻ���������
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // ���㷨�߷ֲ�������NDF��
    float NDF = DistributionGGX(N, H, roughness);

    // ���㼸���ڱκ���
    float G = GeometrySmith(N, V, L, roughness);

    // Cook-Torrance BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // ʹ�� DirectionalLight ��������������
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = (1.0 - F) * object_color * light.colour * light.strength * NdotL;

    // ��������������� DirectionalLight ��Ӱ��
    vec3 ambient = ambient_light_color * ambient_strength;

    // ���������������ɫ
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(final_color, 1.0);
}








