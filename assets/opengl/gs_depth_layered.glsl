#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform int numLayers;
uniform mat4 VP [6];

in vec2 tex_coord_gs[];
out vec2 tex_coord;
out vec3 pos_world_space;

void main()
{
	for (int i = 0; i < numLayers; i++) {
		gl_Layer = i;
		for (int j = 0; j < 3; j++) {
			tex_coord = tex_coord_gs[j];
			pos_world_space = gl_in[j].gl_Position.xyz;
			gl_Position = VP[i] * gl_in[j].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}