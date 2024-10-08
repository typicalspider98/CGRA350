#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;   // 纹理采样器
uniform vec3 light_dir;       // 光源方向（平行光用）
uniform vec3 light_color;     // 光源颜色
uniform vec3 object_color;    // 物体颜色
uniform vec3 view_pos;        // 摄像机位置
uniform float roughness;      // 粗糙度

uniform vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);
uniform float ambient_strength = 0.2;

void main()
{
    // 计算光照方向
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-light_dir); // 平行光的方向

    // 计算漫反射分量
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    // 计算环境光
    vec3 ambient = ambient_light_color * ambient_strength;

    // 采样纹理颜色
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // 最终颜色
    vec3 final_color = (ambient + diffuse) * textureColor;
    FragColor = vec4(final_color, 1.0);
}






