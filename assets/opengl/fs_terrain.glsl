#version 330
 
layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

in vec2 uv;
in vec3 posViewSpace;
in vec3 pos_model_space;

uniform mat4 MV;
uniform sampler2D heightMapSampler;
uniform sampler2D normalSampler;
uniform sampler2D grassDistSampler;
uniform sampler2D rocksSampler;
uniform sampler2D rocksNormalSampler;
uniform sampler2D grassSampler;
uniform sampler2D grassNormalSampler;
uniform sampler2D roadSampler;
uniform sampler2D roadNormalSampler;
uniform mat4 MVInverseTranspose;
uniform vec3 terrainColor;
uniform vec3 grassColorHigh;

void main()
{
	pos_cs = posViewSpace;
	occlusion = vec3(0.f);
	specular = vec3(0.2f);
	light = vec3(0.f);
	
	vec3 normal_model_space = texture(normalSampler, uv).rgb;

	vec3 temp_tangent = vec3(0.f, 0.f, 1.f);
	vec3 bitangent = cross(temp_tangent, normal_model_space);
	vec3 tangent = cross(normal_model_space, bitangent);
	mat3 TBN_view = mat3(normalize((MV * vec4(tangent, 0.f)).xyz), normalize((MV * vec4(bitangent, 0.f)).xyz), normalize((MV * vec4(normal_model_space, 0.f)).xyz));
	
	//normal_cs = normalize((MVInverseTranspose * vec4(normal_model_space, 0.f)).xyz);
  vec3 weights = normal_model_space * normal_model_space;
  vec3 uv_rocks = pos_model_space * 0.02f; //+ vec3(pos_model_space.x * 1.03f, pos_model_space.y * 1.44f, pos_model_space.z * 1.76f) * 0.01f;
  vec3 terrain_col = weights.x * texture(rocksSampler, uv_rocks.zy).rgb +
    weights.y * texture(rocksSampler, uv_rocks.zx).rgb +
    weights.z * texture(rocksSampler, uv_rocks.yx).rgb;

  vec3 normal_ts_rocks = (weights.x * texture(rocksNormalSampler, uv_rocks.zy).rgb +
                  weights.y * texture(rocksNormalSampler, uv_rocks.zx).rgb +
                  weights.z * texture(rocksNormalSampler, uv_rocks.yx).rgb) * 2.f - 1.f;
  vec3 normal_cs_rocks = normalize(TBN_view * normal_ts_rocks);

  vec3 uv_grass = pos_model_space * 0.3f;
  vec3 grass_col = weights.x * texture(grassSampler, uv_grass.yz).rgb +
    weights.y * texture(grassSampler, uv_grass.xz).rgb +
    weights.z * texture(grassSampler, uv_grass.xy).rgb;

  vec3 normal_ts_grass = (weights.x * texture(grassNormalSampler, uv_grass.yz).rgb +
    weights.y * texture(grassNormalSampler, uv_grass.xz).rgb +
    weights.z * texture(grassNormalSampler, uv_grass.xy).rgb) * 2.f - 1.f;
  vec3 normal_cs_grass = normalize(TBN_view * normal_ts_grass);

  vec4 splat_weights = texture(grassDistSampler, uv);

  vec2 uv_road = pos_model_space.xz * 0.7f;
  vec3 road_color = texture(roadSampler, uv_road).rgb;
  vec3 normal_ts_roads = texture(roadNormalSampler, uv_road).rgb * 2.f - 1.f;
  vec3 normal_cs_roads = normalize(TBN_view * normal_ts_roads);

  diffuse = splat_weights.r * grass_col + splat_weights.g * terrain_col + splat_weights.b * road_color;
  normal_cs = splat_weights.r * normal_cs_grass + splat_weights.g * normal_cs_rocks + splat_weights.b * normal_cs_roads;

 /* normal_cs = normalize((MVInverseTranspose * vec4(normal_model_space, 0.f)).xyz);
  diffuse = mix(terrainColor, grassColorHigh, texture(grassDistSampler, uv).r);*/
}