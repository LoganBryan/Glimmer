#version 460 core

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPosition;

//out vec3 fragPos;
//out vec3 normal;

out vec3 N; // Normal in view space 
out vec3 L; // Light dir in view space 
out vec3 V; // View dir in view space 

out vec2 texCoord;

void main()
{
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

	texCoord = aTexCoord;

	mat4 MVP = projection * view * model;
	gl_Position = MVP * vec4(aVertex, 1.0);

//	fragPos = vec3(model * vec4(aVertex, 1.0));
//	normal = mat3(transpose(inverse(model))) * aNormal;
//	texCoord = aTexCoord;
//
//	gl_Position = projection * view * vec4(fragPos, 1.0);
}