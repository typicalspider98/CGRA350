#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;   // ���������
uniform vec3 object_color;    // ������ɫ
uniform vec3 view_pos;        // �����λ��

// Main (directional) light in the scene
struct DirectionalLight 
{ 
    vec3 colour;
    vec3 direction;
    float strength;
};

// �� light ����Ϊ uniform
uniform DirectionalLight light;

// �̶��Ļ��������
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // ��ɫ������
const float ambient_strength = 0.5;                    // ������ǿ��

void main()
{
    // ���㷨�����͹��շ���
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);  // ����ⷽ��
    
    // ���㻷�������
    vec3 ambient = ambient_light_color * ambient_strength;

    // �������������
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.colour * light.strength;

    // ����������ɫ
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // ����������ɫ����������ͷ�������
    vec3 final_color = (ambient + diffuse) * textureColor;
    FragColor = vec4(final_color, 1.0);
}






