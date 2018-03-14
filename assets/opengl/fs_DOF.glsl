#version 330
 
layout (location = 0) out vec3 color;
uniform sampler2DArray gBufferSampler;
uniform sampler2D samplerShaded;
uniform sampler2D samplerBlurred;

uniform vec3 dof_param;

in vec2 uv;

void main()
{		
	float near = dof_param.x;
	float focus_dist = dof_param.y;
	float far = dof_param.z;
	
	float depth = -texture(gBufferSampler, vec3(uv, 0.f)).z;
	
	float is_fully_blurred = float(depth < near || depth > far);
	float is_far = float(depth > focus_dist) * (1.f - is_fully_blurred);
	float is_near = float(depth <= focus_dist) * (1.f - is_fully_blurred);
	float far_blur_factor = (1.f - ((far - depth) / (far - focus_dist))) * is_far;
	float near_blur_factor = (1.f - ((depth - near) / (focus_dist - near))) * is_near;
	float blur_factor = is_fully_blurred + far_blur_factor + near_blur_factor; // only one of the 3 terms is not zero
	
	vec3 scene_color = texture(samplerShaded, uv).rgb;
	vec3 blur_color = texture(samplerBlurred, uv).rgb;
	
	color = (1.f - blur_factor) * scene_color + blur_factor * blur_color;
}