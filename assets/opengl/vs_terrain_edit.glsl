#version 330

uniform int patches_per_dir;
uniform float patch_size;
uniform float terrain_size_half;

void main()
{
	int cp_id = gl_VertexID % 4;
	vec2 uv = vec2(cp_id & 1, cp_id >> 1);
	int patch_id = gl_VertexID / 4;
	vec2 patch_pos = vec2(patch_id % patches_per_dir, patch_id / patches_per_dir);
	gl_Position = vec4((uv.x + patch_pos.x) * patch_size - terrain_size_half, 0.f, (uv.y + patch_pos.y) * patch_size - terrain_size_half, 1.f);
}