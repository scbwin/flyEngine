#version 450

layout(local_size_x = 1024) in;

// vec4 is used instead of vec3 because of alignment restrictions
struct AABB
{
	vec4 bb_min;
	vec4 bb_max;
};

layout (std430, binding = 0) readonly buffer aabb_buffer
{
	AABB aabbs [];
};

layout (std430, binding = 1) writeonly buffer instance_buffer
{
	uint instances []; // visible instance indices
};

struct IndirectInfo
{
  uint _count; //Index count
  uint _primCount; // Number of visible instances, this program only updates this variable
  uint _firstIndex; // Offset into GL_ELEMENT_ARRAY_BUFFER for this mesh
  uint _baseVertex; // Offset into GL_ARRAY_BUFFER for this mesh
  uint _baseInstance; // Not used
  uint _type; // Either GL_UNSIGNED_INT or GL_UNSIGNED_SHORT, depending on the number of vertices of this mesh
};

layout (std430, binding = 2) buffer draw_indirect_info
{
	IndirectInfo indirect_info [];
};

uniform uint ni; // num_instances
uniform vec4 fp [6]; // frustum planes
uniform vec3 cp_w; // camera pos world
uniform uint ml; // max lod
uniform float lm; // lod multiplier
uniform float de; // detail culling error thresh

// Implemented as described in Real-Time Rendering Third Edition
bool aabbOutsideFrustum(uint i, vec3 h, vec4 center)
{
	float e = dot(h, abs(fp[i].xyz));
	float s = dot(center, fp[i]);
	return s - e > 0.f;
}
bool intersectFrustumAABB(vec3 diag)
{
	vec3 h = diag * 0.5f; // Half diagonal vector
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
	vec3 nearest_point = clamp(cp_w,  aabbs[gl_GlobalInvocationID.x].bb_min.xyz, aabbs[gl_GlobalInvocationID.x].bb_max.xyz);
	vec3 to_cam = cp_w - nearest_point;
	float dist2 = dot(to_cam, to_cam);
	vec3 diag = aabbs[gl_GlobalInvocationID.x].bb_max.xyz - aabbs[gl_GlobalInvocationID.x].bb_min.xyz;
	float size2 = dot(diag, diag);
    if (size2 / dist2 > de && intersectFrustumAABB(diag)) {
	  uint lod = min(uint(distance(cp_w, nearest_point) * lm), ml);
	  instances[atomicAdd(indirect_info[lod]._primCount, 1u) + lod * ni] = gl_GlobalInvocationID.x;
	}
  }
}