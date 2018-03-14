#version 330
 
layout (location = 0) in vec2 vertex;

uniform mat4 MVP;
uniform vec2 translation;
uniform vec2 scale;

uniform sampler2D heightMapSampler;
out vec2 uv;

void main()
{
	vec2 v_new = scale * vertex;
	v_new += translation;
	uv = v_new / vec2(textureSize(heightMapSampler, 0));
	float height = texture(heightMapSampler, uv).r;
	gl_Position = MVP * vec4(v_new.x, height, v_new.y, 1.f);
}