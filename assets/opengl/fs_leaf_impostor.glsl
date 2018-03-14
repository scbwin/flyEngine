#version 330
 
layout (location = 0) out vec4 color;

uniform sampler2D diffuseSampler;

in vec2 tex_coord;

void main()
{
	//color = texture(diffuseSampler, tex_coord);
	vec4 col = texture(diffuseSampler, tex_coord);
	if (col.a < 0.5f) {
		discard;
	}
	color = vec4(col.rgb, 1.f);
}