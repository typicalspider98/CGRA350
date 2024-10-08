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

void main()
{
    // ������շ���
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-light_dir); // ƽ�й�ķ���

    // �������������
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    // ���㻷����
    vec3 ambient = ambient_light_color * ambient_strength;

    // ����������ɫ
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // ������ɫ
    vec3 final_color = (ambient + diffuse) * textureColor;
    FragColor = vec4(final_color, 1.0);
}






