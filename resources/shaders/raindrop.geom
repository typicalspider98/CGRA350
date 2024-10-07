#version 450 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

struct Raindrop {
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer Raindrops {
    Raindrop raindrops[];
};

uniform mat4 view;
uniform mat4 projection;
uniform float raindrop_length;

in uint instanceID[];
out float frag_alpha;

void main() {
    uint instanceID = instanceID[0];
    vec4 velocity = normalize(raindrops[instanceID].velocity);

    vec4 start_position = raindrops[instanceID].position;
    vec4 end_position = start_position - velocity * raindrop_length;

    gl_Position = projection * view * start_position;
    frag_alpha = 1;
    EmitVertex();

    gl_Position = projection * view * end_position;
    frag_alpha = 0.5;
    EmitVertex();

    EndPrimitive();
}
