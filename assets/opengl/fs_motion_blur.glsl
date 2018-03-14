#version 330

layout (location = 0) out vec3 fragColor;

uniform sampler2D samplerShaded;
uniform sampler2D samplerDepth;

in vec2 uv;

uniform mat4 VPBeforeTimesVPInverse;

uniform float strength;

void main()
{		
	vec3 pos_ndc = vec3(uv, texture(samplerDepth, uv).x) * 2.f - 1.f;	
	vec4 previousPos = VPBeforeTimesVPInverse * vec4(pos_ndc, 1.f);	
	previousPos.xy /= previousPos.w;
	previousPos.xy = previousPos.xy * 0.5f + 0.5f;
	int numSamples = 8;
	fragColor = texture(samplerShaded, uv).rgb;
	vec2 blur_vec = uv - previousPos.xy;
	for (int i = 1; i < numSamples; i++) {
		vec2 offset = blur_vec * (float(i) / float(numSamples - 1) - 0.5f) * strength;
		fragColor += texture(samplerShaded, uv + offset).rgb;
	}
	fragColor /= float(numSamples);
	
	//float a = 1.f;
	//fragColor = (1.f - a) * fragColor + a * vec3(blur_vec, 0.f);
}