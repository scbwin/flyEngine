#version 330
 
uniform mat4 MV;

uniform sampler2D heightMapSampler;
uniform vec2 grassStart;
uniform int patchSize;
uniform int bladesPerDir;
uniform float bladesPerUnit;
uniform int grassWidthTimesTesselationFactor;
uniform float tesselationFactor;

out vec3 posViewSpace;
out vec3 posModelSpace;

void main()
{	
	vec2 idx = vec2(gl_VertexID % bladesPerDir, gl_VertexID / bladesPerDir) / bladesPerUnit;
	vec2 pos = grassStart + idx;
	posModelSpace = vec3(pos.x, texture(heightMapSampler, pos / vec2(textureSize(heightMapSampler, 0))).r, pos.y);
	posViewSpace = (MV * vec4(posModelSpace, 1.f)).xyz;
}