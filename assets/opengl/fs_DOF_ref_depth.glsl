#version 330
 
layout (location = 0) out float ref_depth;

uniform sampler2DArray gBufferSampler;
uniform sampler2D samplerOld;
uniform float alpha;

in vec2 uv;

void main()
{	
	float depth_old = texelFetch(samplerOld, ivec2(0), 0).r;
	ref_depth = (1.f - alpha) * texture(gBufferSampler, vec3(vec2(0.5f), 0.f)).z + alpha * depth_old;
}