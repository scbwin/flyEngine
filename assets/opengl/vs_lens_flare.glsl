#version 330
 
const vec2 vertices[4] = vec2[](vec2(-1.f, -1.f),  vec2(1.f, -1.f), vec2(-1.f, 1.f),  vec2(1.f, 1.f));

const int MAX_FLARES = 10;
uniform float scale [MAX_FLARES];
uniform float offset [MAX_FLARES];
uniform vec3 uColor[MAX_FLARES];

uniform vec2 scaleFactor;
uniform vec2 lensStart;
uniform vec2 lensEnd;
uniform float weight;

out vec2 tex_coord;
out vec4 col;

void main()
{
	vec2 trans = mix(lensStart, lensEnd, offset[gl_InstanceID % MAX_FLARES]);
	gl_Position = vec4(vertices[gl_VertexID] * scale[gl_InstanceID % MAX_FLARES] * scaleFactor + trans, 0.f, 1.f);
	tex_coord = vertices[gl_VertexID] * 0.5f + 0.5f;
	col = vec4(uColor[gl_InstanceID % MAX_FLARES], weight);
}