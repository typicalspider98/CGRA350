#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // Texture sampler
uniform vec3 object_color;          // Object color
uniform vec3 view_pos;              // Camera position
uniform float roughness;            

// Define DirectionalLight struct
struct DirectionalLight {
    vec3 colour;
    vec3 direction;
    float strength;
};

// Define light as uniform
uniform DirectionalLight light;

// Fixed ambient light parameters
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // White ambient light
const float ambient_strength = 0.2;                    // Ambient light intensity

void main()
{
    // Normalization of normal, light source direction and line of sight direction
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction);  // Use directions in DirectionalLight
    vec3 V = normalize(view_pos - FragPos);

    // Calculate the Angle between the surface normal and the line of sight and the light source
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Calculate the angles theta_r and theta_i
    float theta_i = acos(NdotL);
    float theta_r = acos(NdotV);

    // Calculate alpha and beta
    float alpha = max(theta_i, theta_r);
    float beta = min(theta_i, theta_r);

    // Parameters A and B of the Oren-Nayar model are calculated based on roughness
    float sigma2 = roughness * roughness;
    float A = 1.0 - (sigma2 / (2.0 * (sigma2 + 0.33)));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    // Calculate the projection of the line of sight and light source in the normal direction of the surface
    vec3 V_perp = V - N * NdotV;
    vec3 L_perp = L - N * NdotL;
    float cosPhiDiff = dot(normalize(V_perp), normalize(L_perp));

    // Oren-Nayar diffuse reflection formula
    float rough_diffuse = A + B * max(0.0, cosPhiDiff) * sin(alpha) * tan(beta);
    vec3 diffuse = rough_diffuse * light.colour * light.strength * NdotL * object_color;

    // The ambient light component is not affected by DirectionalLight
    vec3 ambient = ambient_light_color * ambient_strength;

    // Texture sampling and final color
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse) * textureColor;

    FragColor = vec4(final_color, 1.0);
}







