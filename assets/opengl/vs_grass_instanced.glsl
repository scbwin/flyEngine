#version 330
 
uniform mat4 MV;

uniform sampler2D heightMapSampler;
uniform sampler2D grassDistSampler;
uniform sampler2D windSampler;

uniform mat4 P;

uniform vec2 grassStart;
uniform int grassWidthTimesTesselationFactor;
uniform float tesselationFactor;

uniform vec3 grassColorHigh;

//out vec3 posViewSpace;
//out vec3 posModelSpace;

out vec3 posViewSpaceOut;
out vec3 normalViewSpace;
out vec3 color;

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
	vec2 idx = vec2(gl_InstanceID % (grassWidthTimesTesselationFactor), gl_InstanceID / (grassWidthTimesTesselationFactor)) / tesselationFactor;
	vec2 pos = grassStart + idx;
	//vec3 posModelSpace = vec3(pos.x, texture(heightMapSampler, pos / vec2(textureSize(heightMapSampler, 0))).r, pos.y);
	
	mat4 transformation = mat4(0.06f, 0.f, 0.f, 0.f,
								0.f, 0.125f, 0.f, 0.f,
								0.f, 0.f, 1.f, 0.f,
								pos.x, texture(heightMapSampler, pos / vec2(textureSize(heightMapSampler, 0))).r, pos.y, 1.f);
								
	vec4 pos_model_space = transformation * vec4(vertices[gl_VertexID], 0.f, 1.f);
	
	
	posViewSpaceOut = (MV * pos_model_space).xyz;
	normalViewSpace = vec3(0.f, 0.f, 1.f);
	color = grassColorHigh * height_factors[gl_VertexID];
	gl_Position = P * vec4(posViewSpaceOut, 1.f);
}