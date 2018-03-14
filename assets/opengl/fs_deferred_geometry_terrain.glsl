#version 330

layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

uniform sampler2D samplerDiffuseFlat;
uniform sampler2D samplerNormalFlat;
uniform sampler2D samplerDiffuseSteep;
uniform sampler2D samplerNormalSteep;

in vec3 posViewSpace;
in vec2 texCoord;
in mat3 TBN_view;
in vec3 normalModelSpace;
in vec3 posWorldSpace;

uniform float specularExponent;

void main()
{
	pos_cs = posViewSpace;
	occlusion = vec3(0.f);
	specular = vec3(specularExponent);
	light = occlusion;
	
	float weight = smoothstep(0.f, 1.f, normalModelSpace.y);
	
	/*diffuse = mix(texture(samplerDiffuseSteep, texCoord).rgb, texture(samplerDiffuseFlat, texCoord).rgb, weight);
	normal_cs = mix(normalize(TBN_view * (texture(samplerNormalSteep, texCoord).xyz * 2.f - 1.f)),
				 normalize(TBN_view * (texture(samplerNormalFlat, texCoord).xyz * 2.f - 1.f)), weight);*/
	
	vec3 tex_coord = posWorldSpace * 0.08f;
	vec3 weights = normalModelSpace * normalModelSpace; // weights sum up to 1
	vec3 diffuse_steep = weights.x * texture(samplerDiffuseSteep, tex_coord.yz).rgb + 
						 weights.y * texture(samplerDiffuseSteep, tex_coord.zx).rgb +
						 weights.z * texture(samplerDiffuseSteep, tex_coord.xy).rgb;
						 
	vec3 diffuse_flat =  weights.x * texture(samplerDiffuseFlat, tex_coord.yz).rgb + 
						 weights.y * texture(samplerDiffuseFlat, tex_coord.zx).rgb +
						 weights.z * texture(samplerDiffuseFlat, tex_coord.xy).rgb;
						 
	vec3 normal_steep = weights.x * texture(samplerNormalSteep, tex_coord.yz).rgb + 
						 weights.y * texture(samplerNormalSteep, tex_coord.zx).rgb +
						 weights.z * texture(samplerNormalSteep, tex_coord.xy).rgb;
						 
	vec3 normal_flat =  weights.x * texture(samplerNormalFlat, tex_coord.yz).rgb + 
						 weights.y * texture(samplerNormalFlat, tex_coord.zx).rgb +
						 weights.z * texture(samplerNormalFlat, tex_coord.xy).rgb;
						 
	diffuse = mix(diffuse_steep, diffuse_flat, weight);
	normal_cs = mix(normal_steep, normal_flat, weight);
	normal_cs = normalize(TBN_view * (normal_cs * 2.f - 1.f));
}