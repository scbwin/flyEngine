#version 330

layout (points) in;
layout (line_strip, max_vertices = 16) out;

uniform mat4 VP;
in vec3 bb_min_out[];
in vec3 bb_max_out[];

void main()
{
	vec3 bb_min = bb_min_out[0];
	vec3 bb_max = bb_max_out[0];
	gl_Position = VP * vec4(bb_min, 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.x, bb_min.yz), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.xy, bb_min.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.xy, bb_max.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.x, bb_min.y, bb_max.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_min.xy, bb_max.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_min.xy, bb_min.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_min.x, bb_max.y, bb_min.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_min.x, bb_max.y, bb_max.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_min.x, bb_min.y, bb_max.z), 1.f);
	EmitVertex();
	EndPrimitive();
	
	gl_Position = VP * vec4(vec3(bb_min.x, bb_max.y, bb_max.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.x, bb_max.y, bb_max.z), 1.f);
	EmitVertex();
	EndPrimitive();
	
	gl_Position = VP * vec4(vec3(bb_min.x, bb_max.y, bb_min.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.x, bb_max.y, bb_min.z), 1.f);
	EmitVertex();
	EndPrimitive();
	
	gl_Position = VP * vec4(vec3(bb_max.x, bb_min.y, bb_min.z), 1.f);
	EmitVertex();
	gl_Position = VP * vec4(vec3(bb_max.x, bb_min.y, bb_max.z), 1.f);
	EmitVertex();
	EndPrimitive();
}