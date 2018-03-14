#version 330
 
layout (location = 0) out float godRayIntensity;

uniform sampler2D lightingSampler;
uniform vec3 lightPosScreen; // between 0 and 1

uniform float numSamples;
uniform float fade;
uniform float decay;

in vec2 uv; // between 0 and 1

void main()
{		
	float refSamples = 48.f;
	float density = 0.75f;
	vec2 texCoord = uv;
	vec2 deltaTexCoord = (lightPosScreen.xy - texCoord) / numSamples * density;
	float illuminationDecay = 1.f;
	godRayIntensity = 0.f;
	for (float i = 0.f; i < numSamples; i++, texCoord += deltaTexCoord, illuminationDecay *= decay) {
		float brightness = dot(texture(lightingSampler, texCoord).rgb, vec3(0.2126f, 0.7152f, 0.0722f));
		godRayIntensity += brightness * brightness * brightness * illuminationDecay;
	}
	godRayIntensity /= numSamples;
	godRayIntensity *= fade;
}