#version 430

uniform vec3 raindrop_color;

in float frag_alpha;
out vec4 FragColor;

void main() {
    FragColor = vec4(raindrop_color, frag_alpha);
}