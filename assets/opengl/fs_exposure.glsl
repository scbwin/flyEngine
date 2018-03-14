#version 330
 
layout (location = 0) out float exposure;

uniform sampler2D samplerScene;
uniform int level;

uniform sampler2D exposureOld;
uniform float alpha;

in vec2 uv;

void main()
{	
	vec3 color = texelFetch(samplerScene, ivec2(0, 0), level).rgb;
	float old_exposure = texelFetch(exposureOld, ivec2(0, 0), 0).r;
	float avg_brightness = dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
	avg_brightness = max(avg_brightness, 0.01f);
	float new_exposure = 0.5f / avg_brightness;
	exposure = mix(new_exposure, old_exposure, alpha);
}