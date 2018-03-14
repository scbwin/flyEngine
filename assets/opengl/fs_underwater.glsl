#version 330

layout (location = 0) out vec3 fragColor;

uniform sampler2D sceneSampler;
uniform sampler2D blurSampler;
uniform sampler2DArray gBufferSampler;

uniform mat4 VP_inverse;
uniform float waterHeight;

in vec2 uv;

const vec3 water_color = vec3(57.f, 88.f, 121.f) / 255.f;

void main()
{	
	vec4 p_near = VP_inverse * vec4(uv * 2.f - 1.f, -1.f, 1.f);
	p_near.y /= p_near.w;
	float depth = -texture(gBufferSampler, vec3(uv, 0.f)).z;
	float factor = smoothstep(0.f, 15.f, depth);
	vec3 scene_color = texture(sceneSampler, uv).rgb;
	vec3 blur_color = mix(texture(blurSampler, uv).rgb, water_color, 0.3f);
	float t = float(p_near.y <= waterHeight) * factor;
	fragColor = mix(scene_color, blur_color, t);
}