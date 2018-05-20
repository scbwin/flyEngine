#version 450

layout(local_size_x = 1024) in;

struct AABB
{
	vec4 bb_min;
	vec4 bb_max;
};

layout (std430, binding = 0) buffer aabb_buffer
{
	AABB aabbs [];
};

layout (std430, binding = 1) buffer index_buffer
{
	uint indices [];
};

struct IndirectInfo
{
  uint _count;
  uint _primCount;
  uint _firstIndex;
  uint _baseVertex;
  uint _baseInstance;
};

layout (std430, binding = 2) buffer draw_indirect_info
{
	IndirectInfo indirect_info;
};

uniform uint ni; // num_instances
uniform vec4 fp [6]; // frustum planes

bool aabbOutsideFrustum(uint i, vec3 h, vec4 center)
{
	float e = dot(h, abs(fp[i].xyz));
	float s = dot(center, fp[i]);
	return bool(s - e > 0.f);
}

bool intersectFrustumAABB()
{
	vec3 h = (aabbs[gl_GlobalInvocationID.x].bb_max.xyz - aabbs[gl_GlobalInvocationID.x].bb_min.xyz) * 0.5f; // Half diagonal vector
	vec4 center = (aabbs[gl_GlobalInvocationID.x].bb_max + aabbs[gl_GlobalInvocationID.x].bb_min) * 0.5f; // Bounding box center 
	for (uint i = 0; i < 6; i++) {
		if (aabbOutsideFrustum(i, h, center)) {
			return false;
		}
	}
	return true;
}

void main()
{
	if (gl_GlobalInvocationID.x < ni) {
		if (intersectFrustumAABB()) {
			 uint index = atomicAdd(indirect_info._primCount, 1);
			 indices[index] = gl_GlobalInvocationID.x;
		}
	}
}