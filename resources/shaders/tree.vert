#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;
out vec3 ViewDir;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_pos;

void main()
{
    vec3 world_pos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    ViewDir = camera_pos - vec3(world_pos);

    gl_Position = projection * view * vec4(world_pos, 1.0);
}


