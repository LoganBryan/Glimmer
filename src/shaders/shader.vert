#version 460 core

#define MAX_JOINTS 128

layout(location = 0) in vec3 aVertex;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aTangent;
layout(location = 4) in uvec4 aJoints;
layout(location = 5) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool uHasSkinning;

uniform vec3 lightPosition;

layout(std140, binding = 1) uniform JointMatrices
{
	mat4 jointMatrices[MAX_JOINTS];
};

out vec2 texCoord;
out vec3 normal;
out vec4 tangent;

out mat3 TBN;
out vec3 N; // Normal in view space 
out vec3 L; // Light dir in view space 
out vec3 V; // View dir in view space 

void main()
{
	mat4 skinMatrix = mat4(1.0);
	bool useSkinning = uHasSkinning && any(greaterThan(aWeights, vec4(0.0)));
	//gl_Position = viewProjMatrix * modelMatrix * vec4(position, 1.0);

	if (useSkinning) 
	{
		skinMatrix =
			aWeights.x * jointMatrices[aJoints.x] +
			aWeights.y * jointMatrices[aJoints.y] +
			aWeights.z * jointMatrices[aJoints.z] +
			aWeights.w * jointMatrices[aJoints.w];
	}

	vec4 worldPosition ;

	// Modelview matrix
	mat4 mvm = view * model;

	// Transform vertex position to view space
	vec3 positionEye = vec3(mvm * vec4(aVertex, 1.0));

	// View-space normal
	mat3 normalMatrix = transpose(inverse(mat3(model * skinMatrix)));

	// View-space light dir
	L = normalize(lightPosition - positionEye);

	// View vector
	V = normalize(-positionEye);

	N = normalize(normalMatrix * aNormal);

	vec3 T = normalize(normalMatrix * aTangent.xyz);
	vec3 B = normalize(cross(N, T) * aTangent.w);

	TBN = mat3(T, B, N);

	texCoord = aTexCoord;

	normal = mat3(transpose(inverse(model))) * aNormal;
	tangent = vec4(mat3(transpose(inverse(model))) * vec3(aTangent.xyz), 1.0);

	mat4 MVP = projection * view * model;

	if (useSkinning)
		worldPosition = skinMatrix * vec4(aVertex, 1.0);
	else
		worldPosition = vec4(aVertex, 1.0);

	gl_Position = projection * view * model * worldPosition;
}