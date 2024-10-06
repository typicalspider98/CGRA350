#version 330 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

flat in int v_instanceID[];
out vec3 v_color;

uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;

const vec3 dir[] = vec3[](
	vec3(1, 0, 0),
	vec3(-.5, 0, 0),
	vec3(0, 1, 0),
	vec3(0, -.5, 0),
	vec3(0, 0, 1),
	vec3(0, 0, -.5)
);

void main() {
	v_color = abs(dir[v_instanceID[0]]);
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	EmitVertex();
	v_color = abs(dir[v_instanceID[0]]);
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(normalize(dir[v_instanceID[0]]) * 1000, 1.0);
	EmitVertex();
	EndPrimitive();
}
