#version 330 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

flat in int v_instanceID[];

uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;

void main() {
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(v_instanceID[0]-500, 0, 500, 1);
	EmitVertex();

	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(v_instanceID[0]-500, 0, -500, 1);
	EmitVertex();
	EndPrimitive();
}
