#version 330 core

in vec3 v_color;
out vec3 f_color;

void main() {
    gl_FragDepth = gl_FragCoord.z - 0.000001;
    
    f_color = v_color;
}
