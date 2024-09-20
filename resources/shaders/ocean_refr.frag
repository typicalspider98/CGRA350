#version 430 core

in VS_OUT
{
	vec3 wc_pos;
	vec3 wc_normal;
	vec2 tex_coords;
} fs_in;

layout (pixel_center_integer) in vec4 gl_FragCoord;

out vec4 frag_colour;

uniform vec3 wc_camera_pos;
uniform sampler2D tex_S;
uniform sampler2D normalMap;  // ²¨ÎÆ·¨ÏßÌùÍ¼
uniform vec2 viewport_dimensions;

uniform vec3 water_base_colour;
uniform float water_base_colour_amt;

const float delta = 0.05;


// tonemapping and display encoding combined
vec3 tonemap(vec3 linear_rgb)
{
    // no tonemap
	// display encoding
    return pow(linear_rgb, vec3(1.0/2.2)); 
}

void main()
{
	vec3 I_result;

	// calculate normal vectors
	vec3 N = normalize(fs_in.wc_normal);

	// Add: Sample normal map and adjust the normal
	vec3 normalFromMap = texture(normalMap, fs_in.tex_coords).rgb;
	normalFromMap = normalize(normalFromMap * 2.0 - 1.0);  // Convert from [0,1] to [-1,1]

	// Add: Combine the normal from the map with the original normal
	N = normalize(N + normalFromMap);

	// calculate projected texture coordinates (screen/vieewport space)
	vec2 projected_tex_coords = gl_FragCoord.xy / viewport_dimensions;

	// calc new sample coords
	vec2 sample_tex_coords = projected_tex_coords + N.xz * delta;

	// set output/final colour
	vec3 I_refr = water_base_colour_amt * water_base_colour + (1-water_base_colour_amt) * texture(tex_S, sample_tex_coords).rgb;

	frag_colour = vec4(tonemap(I_refr), 1.0);
}