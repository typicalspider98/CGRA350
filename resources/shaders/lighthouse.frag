#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;   // Texture sampler
uniform vec3 object_color;    // Object color
uniform vec3 view_pos;        // Camera position

// Main (directional) light in the scene
struct DirectionalLight 
{ 
    vec3 colour;
    vec3 direction;
    float strength;
};

// Define light as uniform
uniform DirectionalLight light;

// Fixed ambient light parameters
const vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);  // White ambient light
const float ambient_strength = 0.5;                    // Ambient light intensity

void main()
{
    // Calculate normal vector and illumination direction
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);  // Direction of light
    
    // Calculate the ambient light component
    vec3 ambient = ambient_light_color * ambient_strength;

    // Calculate the diffuse reflection component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.colour * light.strength;

    // Sample texture color
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // Calculate the final color, adding the ambient light and the directional light
    vec3 final_color = (ambient + diffuse) * textureColor;
    FragColor = vec4(final_color, 1.0);
}
