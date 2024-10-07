#version 430

struct Raindrop {
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer Raindrops {
    Raindrop raindrops[];
};

uniform mat4 view;
uniform mat4 projection;
//uniform float raindrop_size;

out uint instanceID;

void main() {
    instanceID = gl_InstanceID;
    gl_Position = projection * view * raindrops[gl_InstanceID].position;
    //gl_PointSize = raindrop_size;
}