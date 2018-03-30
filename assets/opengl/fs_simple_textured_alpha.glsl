#version 330
 
layout (location = 0) out vec3 fragmentColor;

in vec3 pos_out;
in vec3 normal_out;
in vec2 uv_out;
in vec3 tangent_out;
in vec3 bitangent_out;

uniform sampler2D ts_diff;
uniform sampler2D ts_alpha;

void main()
{	
	if (texture(ts_alpha, uv_out).r < 0.5) {
		discard;
	}
	fragmentColor = texture(ts_diff, uv_out).rgb;
}