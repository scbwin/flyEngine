#version 330
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tan;
layout (location = 4) in vec3 bitan;

uniform mat4 MV;
uniform mat4 P;
uniform mat4 MVInverseTranspose;

out vec3 posViewSpace;
out vec2 texCoord;
out mat3 TBN_view;
out vec3 normalViewSpace;
out vec3 normalModelSpace;

void main()
{
	posViewSpace = (MV * vec4(vertex, 1.f)).xyz;
	gl_Position = P * vec4(posViewSpace, 1.f);
	texCoord = uv;
	TBN_view = mat3(normalize((MV * vec4(tan, 0.f)).xyz), normalize((MV * vec4(bitan, 0.f)).xyz), normalize((MV * vec4(normal, 0.f)).xyz));
	normalModelSpace = normal;
	normalViewSpace = normalize((MVInverseTranspose * vec4(normal, 1.f)).xyz);
}