#version 330

layout (points) in;
layout (triangle_strip, max_vertices = 7) out;

uniform mat4 MV;
uniform mat4 P;
uniform float grassDistance;

in vec3 posViewSpace [];
in vec3 posModelSpace [];

out vec3 posViewSpaceOut;
out vec3 normalViewSpace;
out vec3 color;

uniform sampler2D heightMapSampler;
uniform sampler2D grassDistSampler;
uniform vec2 windDir;
uniform float windStrength;
uniform float windFrequency;

uniform vec3 grassColorHigh;
uniform float grassHeight;

uniform float time;

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898f, 78.233f))) * 43758.5453f);
}

vec2 interpolate(vec2 t)
{
	return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

float noise(vec2 p)
{
	vec2 start = floor(p);
	vec2 end = start + 1.f;
	vec2 weights = interpolate(p - start);
	float a = hash(start);
	float b = hash(vec2(end.x, start.y));
	float c = hash(vec2(start.x, end.y));
	float d = hash(end);
	return mix(mix(a, b, weights.x), mix(c, d, weights.x), weights.y);
}

const vec2 vertices [7] = vec2 [] (
	vec2(0.f),
	vec2(0.13f, 0.0f),
	vec2(0.07f, 0.3f),
	vec2(0.2f, 0.28f),
	vec2(0.14f, 0.6f),
	vec2(0.24f, 0.57f),
	vec2(0.35f, 0.75f)
);

const float height_factors [7] = float [] (
	0.f, 0.f, 0.1332f, 0.1144f, 0.612f, 0.5467f, 1.f
);

void main()
{
	float dist_cs = length(posViewSpace[0]);
	if (dist_cs > grassDistance) { // culling
		return;
	}
	float grass_density = texture(grassDistSampler, posModelSpace[0].xz / vec2(textureSize(grassDistSampler, 0))).r;
	if (grass_density < 0.3f) {
		return;
	}
	float height = grassHeight * clamp(hash(posModelSpace[0].xz * 8.f), 0.7f, 1.f) * grass_density * (1.f - smoothstep(grassDistance * 0.85f, grassDistance, dist_cs));

	vec2 offset = vec2(hash(posModelSpace[0].xz), hash(posModelSpace[0].xz * 2.f)) * 0.05f;
	mat4 t = mat4(1.f, 0.f, 0.f, 0.f,
						0.f, 1.f, 0.f, 0.f,
						0.f, 0.f, 1.f, 0.f,
						posModelSpace[0].x + offset.x, texture(heightMapSampler, posModelSpace[0].xz / vec2(textureSize(heightMapSampler, 0))).r, posModelSpace[0].z + offset.y, 1.f);
	float radians = hash(posModelSpace[0].xz) * 3.14f;
	float a = sin(radians);
	float b = cos(radians);
	mat4 rotation_mat_y =  mat4(b, 0.f, -a, 0.f,
							  0.f, 1.f, 0.f, 0.f,
							  a, 0.f, b, 0.f,
							  0.f, 0.f, 0.f, 1.f);
	radians = hash(posModelSpace[0].xz * 8.f) * 1.4f - 0.7f;
	a = sin(radians);
	b = cos(radians);
	mat4 rotation_mat_x =  mat4(1.f, 0.f, 0.f, 0.f,
							  0.f, b, a, 0.f,
							  0.f, -a, b, 0.f,
							  0.f, 0.f, 0.f, 1.f);
		
	radians = hash(posModelSpace[0].xz * 16.f) * 1.4f - 0.7f;
	a = sin(radians);
	b = cos(radians);
	mat4 rotation_mat_z = mat4(b, a, 0.f, 0.f,
								-a, b, 0.f, 0.f,
								0.f, 0.f, 1.f, 0.f,
								0.f, 0.f, 0.f, 1.f);
								

	mat4 scale_matrix = mat4(0.075f * grass_density, 0.f, 0.f, 0.f,
							0.f, height, 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.f, 0.f, 0.f, 1.f);
	mat4 transform = t *  rotation_mat_x * rotation_mat_z * rotation_mat_y * scale_matrix; // places the grass blade on the terrain

	vec2 uv = posModelSpace[0].xz;
	float time_factor = time * windFrequency * 3.f;
	vec2 time_f = time * windFrequency * 64.f * windDir;
	float n =  noise(uv * 4.f - time_f) + noise(uv * 8.f - time_f);
	vec2 wind = windDir * n * windStrength * 0.01f;
	vec3 verts [7];
	vec3 colors[7];
	for (int i = 0; i < vertices.length(); i++) {
		colors[i] = grassColorHigh * max(height_factors[i], 0.25f);
		verts[i] = (transform * vec4(vertices[i], 0.f, 1.f)).xyz;
		verts[i].xz += wind * height_factors[i];
		verts[i] = (MV * vec4(verts[i], 1.f)).xyz;
	}
	
	vec3 normals_view_space [7];	
	for (int i = 2; i < verts.length(); i++) {
		normals_view_space[i] = normalize(cross(verts[i] - verts[i - 2], verts[i] - verts[i - 1]));
	}
	normals_view_space[0] = normals_view_space[2];
	normals_view_space[1] = normals_view_space[2];
	
	for (int i = 0; i < verts.length(); i++) {
		posViewSpaceOut = verts[i];
		gl_Position = P * vec4(posViewSpaceOut, 1.f);
		normalViewSpace = normals_view_space[i] * (-sign(dot(normals_view_space[i], normalize(verts[i]))));
		color = colors[i];
		EmitVertex();
	}
	EndPrimitive();
}