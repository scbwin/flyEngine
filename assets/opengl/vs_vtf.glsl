#version 330
 
layout (location = 0) in vec2 vertex;

uniform mat4 MV;
uniform mat4 P;
uniform vec2 translation;
uniform vec2 scale;
uniform mat4 MVInverseTranspose;


uniform sampler2D heightMapSampler;
out vec2 uv;
out vec3 posViewSpace;
//out vec3 normalViewSpace;

void main()
{
	vec2 v_new = scale * vertex;
	v_new += translation;
	uv = v_new / vec2(textureSize(heightMapSampler, 0));
	float height = texture(heightMapSampler, uv).r;
	vec3 pos_model_space = vec3(v_new.x, height, v_new.y);
	posViewSpace = (MV * vec4(pos_model_space, 1.f)).xyz;
	gl_Position = P * vec4(posViewSpace, 1.f);
	
	/*vec2 texel_size =  1.f / vec2(textureSize(heightMapSampler, 0));
	float height_left = texture(heightMapSampler, uv - vec2(texel_size.x, 0.f)).r;
	float height_right = texture(heightMapSampler, uv + vec2(texel_size.x, 0.f)).r;
	float height_down = texture(heightMapSampler, uv - vec2(0.f, texel_size.y)).r;
	float height_up = texture(heightMapSampler, uv + vec2(0.f, texel_size.y)).r;
	
	vec3 normal_model_space = normalize(vec3(height_left - height_right, 2.f, height_down - height_up));
	normalViewSpace = normalize((MVInverseTranspose * vec4(normal_model_space, 1.f)).xyz);*/
}