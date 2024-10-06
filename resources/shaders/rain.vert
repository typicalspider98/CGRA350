#version 430
layout(std430, binding = 0) buffer Raindrops {
    vec3 positions[];
};

uniform mat4 view;
uniform mat4 projection;
uniform uint instanceID;

void main() {
    vec3 position = positions[gl_InstanceID];
    gl_Position = projection * view * vec4(position, 1.0);
    gl_PointSize = 5.0;
}