#version 330 core

in vec3 Normal;
in vec2 TexCoord;
in vec3 ViewDir;
out vec4 FragColor;

uniform sampler2D texture1;   // ���������
uniform vec3 light_dir;       // ��Դ����ƽ�й��ã�
uniform vec3 light_color;     // ��Դ��ɫ

uniform vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);
uniform float ambient_strength = 0.2;
uniform float alphaCutoff = 0.25;

uniform sampler2D normalMap;        // ������ͼ
uniform sampler2D specularMap;      // �ض�������ͼ


void main()
{
    // ͸���Ȳü�
    vec4 textureColor = texture(texture1, TexCoord);
    if (textureColor.a < alphaCutoff) {
        discard;
    }
    
    vec3 albedoColor = textureColor.rgb;
    vec3 norm = normalize(Normal);

    vec3 lightDir = normalize(-light_dir);
    vec3 viewDir = normalize(ViewDir);

    // ���������
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * albedoColor;

    // ������
    vec3 ambient = ambient_light_color * ambient_strength * albedoColor;

    // ��������
    float specularStrength = texture(specularMap, TexCoord).r;
    float spec = pow(max(dot(viewDir, reflect(-lightDir, norm)), 0.0), 16.0);
    vec3 specular = specularStrength * spec * vec3(1);

    vec3 final_color = (ambient + diffuse + specular) * light_color;
    FragColor = vec4(final_color, 1);
}

