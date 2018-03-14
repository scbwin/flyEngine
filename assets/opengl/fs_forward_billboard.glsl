#version 330

layout (location = 0) out vec4 color;

uniform sampler2D alphaSampler;
uniform sampler2D samplerPosCs;

uniform float alpha;

uniform int smoothParticles;

in float fade_out;
in vec2 uv;
in float view_space_z;
in vec3 particle_color;

void main()
{
	vec2 screen_uv = gl_FragCoord.xy / textureSize(samplerPosCs,0);
	float scene_z = texture(samplerPosCs, screen_uv).z;
	float smooth_factor = smoothParticles == 1 ? clamp(abs(scene_z - view_space_z) / 0.1f, 0.f, 1.f) : 1.f;
	color = vec4(particle_color, texture(alphaSampler, uv).r * fade_out * smooth_factor * alpha);
}