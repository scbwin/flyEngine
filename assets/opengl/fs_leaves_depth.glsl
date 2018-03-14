#version 330

uniform sampler2D samplerDiffuse;

in vec2 uv;

void main()
{
	vec4 col = texture(samplerDiffuse, uv);
	if (col.a < 0.5f) {
		discard;
	}
}