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

uniform sampler2D normalMap;        // 法线贴图
uniform sampler2D specularMap;      // 特定光照贴图
uniform sampler2D alphaMap;         // alpha裁剪贴图

void main()
{
    vec3 norm = texture(normalMap, TexCoord).rgb * 2.0 - 1.0;  // 法线贴图
    norm = normalize(norm);

    vec3 lightDir = normalize(-light_dir);
    vec3 viewDir = normalize(view_pos - FragPos);

    // 透明度裁剪
    float alpha = texture(alphaMap, TexCoord).r;
    if (alpha < 0.5) discard;

    // 漫反射分量
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    // 环境光
    vec3 ambient = ambient_light_color * ambient_strength;

    // 反射光分量
    vec3 specularColor = texture(specularMap, TexCoord).rgb;
    vec3 specular = specularColor * light_color * pow(max(dot(viewDir, reflect(-lightDir, norm)), 0.0), 16.0);

    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(final_color, alpha);
}







