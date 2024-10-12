#version 330 core

in vec3 Normal;
in vec2 TexCoord;
in vec3 ViewDir;
out vec4 FragColor;

uniform sampler2D texture1;   // Texture sampler
//uniform vec3 light_dir;       // Direction of light source
//uniform vec3 light_color;     // Light source color

// Main (directional) light in the scene
struct DirectionalLight 
{ 
    vec3 colour;
    vec3 direction;
    float strength;
};

// Define light as uniform
uniform DirectionalLight light;

uniform vec3 ambient_light_color = vec3(1.0, 1.0, 1.0);
uniform float ambient_strength = 0.2;
uniform float alphaCutoff = 0.25;

uniform sampler2D normalMap;        // Normal map
uniform sampler2D specularMap;      // Specific light map


void main()
{
    // Transparency clipping
    vec4 textureColor = texture(texture1, TexCoord);
    if (textureColor.a < alphaCutoff) {
        discard;
    }
    
    vec3 albedoColor = textureColor.rgb;
    vec3 norm = normalize(Normal);

    vec3 lightDir = normalize(-light.direction);
    vec3 viewDir = normalize(ViewDir);

    // Diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * albedoColor * light.strength;

    // Ambient light
    vec3 ambient = ambient_light_color * ambient_strength * albedoColor;

    // Reflected light component
    float specularStrength = texture(specularMap, TexCoord).r;
    float spec = pow(max(dot(viewDir, reflect(-lightDir, norm)), 0.0), 16.0);
    vec3 specular = specularStrength * spec * vec3(1);

    vec3 final_color = (ambient + diffuse + specular) * light.colour;
    FragColor = vec4(final_color, 1);
}

