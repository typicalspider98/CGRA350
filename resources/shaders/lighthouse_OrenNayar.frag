#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // ���������
uniform vec3 object_color;          // ������ɫ
uniform vec3 view_pos;              // �����λ��
uniform float roughness;            // �ֲڶ�

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

void main()
{
    // ���ߡ���Դ��������߷���ĵ�λ��
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction);  // ʹ�� DirectionalLight �еķ���
    vec3 V = normalize(view_pos - FragPos);

    // ������淨�������ߺ͹�Դ�ļн�
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // ����Ƕ� theta_r �� theta_i
    float theta_i = acos(NdotL);
    float theta_r = acos(NdotV);

    // ���� alpha �� beta
    float alpha = max(theta_i, theta_r);
    float beta = min(theta_i, theta_r);

    // ���ݴֲڶȼ��� Oren-Nayar ģ�͵Ĳ��� A �� B
    float sigma2 = roughness * roughness;
    float A = 1.0 - (sigma2 / (2.0 * (sigma2 + 0.33)));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    // �������ߺ͹�Դ�ڱ��淨�߷����ͶӰ
    vec3 V_perp = V - N * NdotV;
    vec3 L_perp = L - N * NdotL;
    float cosPhiDiff = dot(normalize(V_perp), normalize(L_perp));

    // Oren-Nayar �����乫ʽ
    float rough_diffuse = A + B * max(0.0, cosPhiDiff) * sin(alpha) * tan(beta);
    vec3 diffuse = rough_diffuse * light.colour * light.strength * NdotL * object_color;

    // ��������������� DirectionalLight ��Ӱ��
    vec3 ambient = ambient_light_color * ambient_strength;

    // ���������������ɫ
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse) * textureColor;

    FragColor = vec4(final_color, 1.0);
}







