#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // 纹理采样器
uniform vec3 object_color;          // 物体颜色
uniform vec3 view_pos;              // 摄像机位置
uniform float roughness;            // 粗糙度
uniform float metalness;            // 金属度
uniform float reflectivity;         // 反射率

// 定义 DirectionalLight 结构体
struct DirectionalLight {
    vec3 colour;
    vec3 direction;
    float strength;
};

// 将 light 定义为 uniform
uniform DirectionalLight light;

// 固定的环境光参数
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // 白色环境光
const float ambient_strength = 0.2;                    // 环境光强度

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
    // 法线、光源方向和视线方向的单位化
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction); // 使用 DirectionalLight 中的方向
    vec3 V = normalize(view_pos - FragPos);
    vec3 H = normalize(V + L);

    // 计算菲涅尔反射率
    vec3 F0 = mix(vec3(0.04), object_color, metalness);  // 使用基于金属度的基础反射率
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // 计算法线分布函数（NDF）
    float NDF = DistributionGGX(N, H, roughness);

    // 计算几何遮蔽函数
    float G = GeometrySmith(N, V, L, roughness);

    // Cook-Torrance BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // 使用 DirectionalLight 计算的漫反射分量
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = (1.0 - F) * object_color * light.colour * light.strength * NdotL;

    // 环境光分量，不受 DirectionalLight 的影响
    vec3 ambient = ambient_light_color * ambient_strength;

    // 纹理采样和最终颜色
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(final_color, 1.0);
}








