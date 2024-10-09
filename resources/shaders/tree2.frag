#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;   // ���������
uniform vec3 light_dir;       // ��Դ����ƽ�й��ã�
uniform vec3 light_color;     // ��Դ��ɫ
uniform vec3 object_color;    // ������ɫ
uniform vec3 view_pos;        // �����λ��
uniform float roughness;      // �ֲڶ�

uniform vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);
uniform float ambient_strength = 0.2;

uniform sampler2D normalMap;        // ������ͼ
uniform sampler2D specularMap;      // �ض�������ͼ
uniform sampler2D alphaMap;         // alpha�ü���ͼ

void main()
{
    vec3 norm = texture(normalMap, TexCoord).rgb * 2.0 - 1.0;  // ������ͼ
    norm = normalize(norm);

    vec3 lightDir = normalize(-light_dir);
    vec3 viewDir = normalize(view_pos - FragPos);

    // ͸���Ȳü�
    float alpha = texture(alphaMap, TexCoord).r;
    if (alpha < 0.5) discard;

    // ���������
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    // ������
    vec3 ambient = ambient_light_color * ambient_strength;

    // ��������
    vec3 specularColor = texture(specularMap, TexCoord).rgb;
    vec3 specular = specularColor * light_color * pow(max(dot(viewDir, reflect(-lightDir, norm)), 0.0), 16.0);

    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(final_color, alpha);
}







