#version 460 core

layout(location = 0) in vec3 aVertex;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPosition;

out vec2 texCoord;
out vec3 normal;
out vec4 tangent;

out mat3 TBN;
out vec3 N; // Normal in view space 
out vec3 L; // Light dir in view space 
out vec3 V; // View dir in view space 

void main()
{
	//gl_Position = viewProjMatrix * modelMatrix * vec4(position, 1.0);


	// Modelview matrix
	mat4 mvm = view * model;

	// Transform vertex position to view space
	vec3 positionEye = vec3(mvm * vec4(aVertex, 1.0));

	// View-space normal
	N = normalize(mat3(mvm) * aNormal);

	// View-space light dir
	L = normalize(lightPosition - positionEye);

	// View vector
	V = normalize(-positionEye);

	vec3 T = normalize(mat3(model) * aTangent.xyz);
	vec3 B = normalize(cross(N, T) * aTangent.w);

	TBN = mat3(T, B, N);

	texCoord = aTexCoord;

	normal = mat3(transpose(inverse(model))) * aNormal;
	tangent = vec4(mat3(transpose(inverse(model))) * vec3(aTangent.xyz), 1.0);

	mat4 MVP = projection * view * model;
	gl_Position = MVP * vec4(aVertex, 1.0);
}