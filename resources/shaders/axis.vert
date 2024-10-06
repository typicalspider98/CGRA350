#version 330 core

uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;

flat out int v_instanceID;

void main() {
    v_instanceID = gl_InstanceID;
}
