#version 330

layout (location = 0) in vec3 bb_min;
layout (location = 1) in vec3 bb_max;

out vec3 bb_min_out;
out vec3 bb_max_out;

void main()
{
	bb_min_out = bb_min;
	bb_max_out = bb_max;
}