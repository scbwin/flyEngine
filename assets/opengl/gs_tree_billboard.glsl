#version 330

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform vec3 camUpWorld;
uniform vec3 camRightWorld;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

in vec3 pos_model_space [];
in vec3 billboard_size [];

out vec4 pos_view_space;
out vec2 uv;

const vec2 uvs [4] = vec2 [] (vec2(0.f, 0.f), vec2(1.f, 0.f), vec2(0.f, 1.f), vec2(1.f, 1.f));

void main()
{
	vec3 billboard_bottom_world = (M * vec4(pos_model_space[0], 1.f)).xyz;
	
	vec3 verts_world [4];
	verts_world[0] = billboard_bottom_world - camRightWorld * billboard_size[0];
	verts_world[1] = billboard_bottom_world + camRightWorld * billboard_size[0];
	verts_world[2] = billboard_bottom_world - camRightWorld * billboard_size[0] + camUpWorld * billboard_size[0];
	verts_world[3] = billboard_bottom_world + camRightWorld * billboard_size[0] + camUpWorld * billboard_size[0];
	
	for (int i = 0; i < 4; i++) {
		pos_view_space = V * vec4(verts_world[i], 1.f);
		uv = uvs[i];
		gl_Position = P * pos_view_space;
		EmitVertex();
	}
	EndPrimitive();
}