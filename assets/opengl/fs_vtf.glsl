#version 330
 
layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

uniform vec3 col;

in vec2 uv;
in vec3 posViewSpace;
//in vec3 normalViewSpace;

uniform mat4 MVInverseTranspose;
uniform sampler2D heightMapSampler;

void main()
{
	pos_cs = posViewSpace;
	//normal_cs = normalViewSpace;
	diffuse = vec3(1.f);
	occlusion = vec3(0.f);
	specular = vec3(64.f);
	light = vec3(0.f);
		
	vec2 texel_size =  1.f / vec2(textureSize(heightMapSampler, 0));
	float height_left = texture(heightMapSampler, uv - vec2(texel_size.x, 0.f)).r;
	float height_right = texture(heightMapSampler, uv + vec2(texel_size.x, 0.f)).r;
	float height_down = texture(heightMapSampler, uv - vec2(0.f, texel_size.y)).r;
	float height_up = texture(heightMapSampler, uv + vec2(0.f, texel_size.y)).r;
	
	vec3 normal_model_space = normalize(vec3(height_left - height_right, 2.f, height_down - height_up));
	normal_cs = normalize((MVInverseTranspose * vec4(normal_model_space, 1.f)).xyz);
}