#version 330

in vec3 Position;
out vec2 TexCoord;

void main() 
{
    gl_Position = vec4(Position, 1.0);
    TexCoord = 0.5 * Position.xy + vec2(0.5);
}