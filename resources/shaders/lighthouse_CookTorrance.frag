#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;         // Texture sampler
uniform vec3 object_color;          // Object color
uniform vec3 view_pos;              // Camera position
uniform float roughness;            
uniform float metalness;            
uniform float reflectivity;         

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

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Normalization of normal, light source direction and line of sight direction
    vec3 N = normalize(Normal);
    vec3 L = normalize(-light.direction); // Use directions in DirectionalLight
    vec3 V = normalize(view_pos - FragPos);
    vec3 H = normalize(V + L);

    // Calculate Fresnel reflectance
    vec3 F0 = mix(vec3(0.04), object_color, metalness);  // Use base reflectance based on metallicity
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Calculate Normal Distribution Function (NDF)
    float NDF = DistributionGGX(N, H, roughness);

    // Calculate the geometric masking function
    float G = GeometrySmith(N, V, L, roughness);

    // Cook-Torrance BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // Diffuse reflection component calculated using DirectionalLight
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = (1.0 - F) * object_color * light.colour * light.strength * NdotL;

    // The ambient light component is not affected by DirectionalLight
    vec3 ambient = ambient_light_color * ambient_strength;

    // Texture sampling and final color
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    vec3 final_color = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(final_color, 1.0);
}








