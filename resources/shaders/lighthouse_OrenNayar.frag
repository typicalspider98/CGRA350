#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // 纹理采样器
uniform vec3 object_color;          // 物体颜色
uniform vec3 view_pos;              // 摄像机位置
uniform float roughness;            // 粗糙度

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

void main()
{
    // 法线、光源方向和视线方向的单位化
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction);  // 使用 DirectionalLight 中的方向
    vec3 V = normalize(view_pos - FragPos);

    // 计算表面法线与视线和光源的夹角
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // 计算角度 theta_r 和 theta_i
    float theta_i = acos(NdotL);
    float theta_r = acos(NdotV);

    // 计算 alpha 和 beta
    float alpha = max(theta_i, theta_r);
    float beta = min(theta_i, theta_r);

    // 根据粗糙度计算 Oren-Nayar 模型的参数 A 和 B
    float sigma2 = roughness * roughness;
    float A = 1.0 - (sigma2 / (2.0 * (sigma2 + 0.33)));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    // 计算视线和光源在表面法线方向的投影
    vec3 V_perp = V - N * NdotV;
    vec3 L_perp = L - N * NdotL;
    float cosPhiDiff = dot(normalize(V_perp), normalize(L_perp));

    // Oren-Nayar 漫反射公式
    float rough_diffuse = A + B * max(0.0, cosPhiDiff) * sin(alpha) * tan(beta);
    vec3 diffuse = rough_diffuse * light.colour * light.strength * NdotL * object_color;

    // 环境光分量，不受 DirectionalLight 的影响
    vec3 ambient = ambient_light_color * ambient_strength;

    // 纹理采样和最终颜色
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse) * textureColor;

    FragColor = vec4(final_color, 1.0);
}







