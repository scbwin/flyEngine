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
	uint visible_instances []; // visible instance indices
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
uniform vec3 cp_w; // camera pos world
uniform uint ml; // max lod
uniform float de; // detail culling error thresh
uniform float lr; // lod range

#define ID gl_GlobalInvocationID.x

void main()
{
  if (ID < ni) {
    vec3 nearest_point = clamp(cp_w,  aabbs[ID].bb_min.xyz, aabbs[ID].bb_max.xyz);
    vec3 to_cam = cp_w - nearest_point;
    float dist2 = dot(to_cam, to_cam);
    vec3 diag = aabbs[ID].bb_max.xyz - aabbs[ID].bb_min.xyz;
    float size2 = dot(diag, diag);
    float ratio = size2 / dist2;
    if (ratio > de) {
      float alpha = 1.f - min((ratio - de) / lr, 1.f);
      uint lod = uint(round(alpha * ml));
      visible_instances[lod * ni + atomicAdd(indirect_info[lod]._primCount, 1u)] = ID;
    }
  }
}