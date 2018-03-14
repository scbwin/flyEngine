#version 330
 
layout (location = 0) in vec2 vertex;

uniform mat4 MVP;
uniform vec2 pos;
uniform float scale;

uniform sampler2D heightMapSampler;

out vec2 uv;
out vec3 posViewSpace;

void main()
{	
	vec2 pos = vertex * scale + pos;
	uv = pos / vec2(textureSize(heightMapSampler, 0));
	float height = texture(heightMapSampler, uv).r;
	vec3 pos_model_space = vec3(pos.x, height, pos.y);
	gl_Position = MVP * vec4(pos_model_space, 1.f);
}