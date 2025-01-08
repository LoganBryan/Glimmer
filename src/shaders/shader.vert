#version 460 core

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPosition;

out vec3 fragPos;
out vec3 normal;

out mat3 TBN;
out vec3 N; // Normal in view space 
out vec3 L; // Light dir in view space 
out vec3 V; // View dir in view space 

out vec2 texCoord;
out vec4 tangent;

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

	vec3 T = normalize(mat3(model) * aTangent.xyz);
	vec3 B = normalize(cross(N, T) * aTangent.w);

	TBN = mat3(T, B, N);

	texCoord = aTexCoord;
	tangent = vec4(mat3(transpose(inverse(model))) * vec3(aTangent.xyz), 1.0);

//	// edge vectors in view space
//	vec3 v0 = vec3(mvm * vec4(aVertex, 1.0));
//	vec3 v1 = vec3(mvm * vec4(aVertex + vec3(1.0, 0.0, 0.0), 1.0));
//	vec3 v2 = vec3(mvm * vec4(aVertex + vec3(0.0, 1.0, 0.0), 1.0));
//
//	vec3 edge1 = v1 - v0;
//	vec3 edge2 = v2 - v0;
//
//	// texture coord differences
//	vec2 deltaUV1 = aTexCoord - vec2(0.0, 0.0);
//	vec2 deltaUV2 = aTexCoord - vec2(1.0, 0.0);
//
//	// determinant
//	float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
//
//	// tangent and bitangent
//	T = normalize(f * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
//	B = normalize(f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));

	// Probably don't need these
	fragPos = vec3(model * vec4(aVertex, 1.0));
	normal = mat3(transpose(inverse(model))) * aNormal;

	mat4 MVP = projection * view * model;
	gl_Position = MVP * vec4(aVertex, 1.0);
}