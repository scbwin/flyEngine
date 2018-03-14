#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 VP[6];
uniform int numLayers;

in vec4 pos_world_space [];

void main()
{
	for (int i = 0; i < numLayers; i++) {
		gl_Layer = i;
		for (int j = 0; j < 3; j++) {
			gl_Position = VP[i] * pos_world_space[j];
			EmitVertex();
		}
		EndPrimitive();
	}
}