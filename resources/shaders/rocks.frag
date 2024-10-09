#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;   // 纹理采样器
uniform vec3 object_color;    // 物体颜色
uniform vec3 view_pos;        // 摄像机位置

// Main (directional) light in the scene
struct DirectionalLight 
{ 
    vec3 colour;
    vec3 direction;
    float strength;
};

// 将 light 定义为 uniform
uniform DirectionalLight light;

// 固定的环境光参数
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // 白色环境光
const float ambient_strength = 0.5;                    // 环境光强度

void main()
{
    // 计算法向量和光照方向
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);  // 方向光方向
    
    // 计算环境光分量
    vec3 ambient = ambient_light_color * ambient_strength;

    // 计算漫反射分量
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.colour * light.strength;

    // 采样纹理颜色
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // 计算最终颜色，将环境光和方向光相加
    vec3 final_color = (ambient + diffuse) * textureColor;
    FragColor = vec4(final_color, 1.0);
}






