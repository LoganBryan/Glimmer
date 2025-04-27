#version 460 core

#define MAX_JOINTS 128

layout(location = 0) in vec3 aVertex;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aTangent;
layout(location = 4) in uvec4 aJoints;
layout(location = 5) in vec4 aWeights;

//uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool uHasSkinning;

struct Transform
{
	vec3 position;
	vec4 rotation;
	vec3 scale;
	float _pad;
};

struct SceneObject
{
	Transform instanceTransform;
	mat4 nodeTransform;  
	uint materialIndex;
	uint skinIndex;
	uint flags;
	uint _pad;
};

layout(std140, binding = 1) uniform JointMatrices
{
	mat4 jointMatrices[MAX_JOINTS];
};

layout(std430, binding = 2) readonly buffer InstanceBuffer
{
	SceneObject instances[];
} instanceData;
out vec3 fragPosView;
out vec2 texCoord;
out vec3 normal;
out vec4 tangent;

out mat3 TBN;
out vec3 N; // Normal in view space 
out vec3 V; // View dir in view space 

mat4 quatToMat4(vec4 q)
{
	float qx = q.x;
    float qy = q.y;
    float qz = q.z;
    float qw = q.w;
    
    mat4 result = mat4(1.0);
    
    float qxx = qx * qx;
    float qyy = qy * qy;
    float qzz = qz * qz;
    float qxz = qx * qz;
    float qxy = qx * qy;
    float qyz = qy * qz;
    float qwx = qw * qx;
    float qwy = qw * qy;
    float qwz = qw * qz;
    
    result[0][0] = 1.0 - 2.0 * (qyy + qzz);
    result[0][1] = 2.0 * (qxy - qwz);
    result[0][2] = 2.0 * (qxz + qwy);
    
    result[1][0] = 2.0 * (qxy + qwz);
    result[1][1] = 1.0 - 2.0 * (qxx + qzz);
    result[1][2] = 2.0 * (qyz - qwx);
    
    result[2][0] = 2.0 * (qxz - qwy);
    result[2][1] = 2.0 * (qyz + qwx);
    result[2][2] = 1.0 - 2.0 * (qxx + qyy);
    
    return result;
}

void main()
{
	uint id = gl_BaseInstance + gl_InstanceID;
	SceneObject currentInst = instanceData.instances[id];

	mat4 M = currentInst.nodeTransform;

//	Transform dTransform = currentInst.instanceTransform;
//	mat4 scaleMat = mat4(
//        vec4(dTransform.scale.x, 0.0, 0.0, 0.0),
//        vec4(0.0, dTransform.scale.y, 0.0, 0.0),
//        vec4(0.0, 0.0, dTransform.scale.z, 0.0),
//        vec4(0.0, 0.0, 0.0, 1.0)
//    );
//
//	mat4 rotMat = quatToMat4(dTransform.rotation);
//
//    mat4 transMat = mat4(
//        vec4(1.0, 0.0, 0.0, 0.0),
//        vec4(0.0, 1.0, 0.0, 0.0),
//        vec4(0.0, 0.0, 1.0, 0.0),
//        vec4(dTransform.position, 1.0)
//    );
//
//	mat4 instMat = transMat * rotMat * scaleMat;
//	mat4 nodeMat = currentInst.nodeTransform;
//	mat4 M = instMat * nodeMat;

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

	vec4 worldPosition;
	
	if (useSkinning)
		worldPosition = M * skinMatrix * vec4(aVertex, 1.0);
	else
		worldPosition = M * vec4(aVertex, 1.0);

	// Modelview matrix
	mat4 mvm = view * M;

	// Transform vertex position to view space
	vec3 positionEye = vec3(mvm * (useSkinning ? skinMatrix * vec4(aVertex, 1.0) : vec4(aVertex, 1.0)));

	// View-space normal
	mat3 normalMatrix = transpose(inverse(mat3(M)));
	if (useSkinning)
	{
		normalMatrix *= mat3(skinMatrix);
	}

	// View vector
	V = normalize(-positionEye);

	N = normalize(mat3(view) * normalMatrix * aNormal);
	vec3 T = normalize(mat3(view) * normalMatrix * aTangent.xyz);
	vec3 B = normalize(cross(N, T) * aTangent.w);

	TBN = mat3(T, B, N);

	texCoord = aTexCoord;

	normal = normalMatrix * aNormal;
	tangent = vec4(normalMatrix * aTangent.xyz, aTangent.w);

	fragPosView = positionEye;
	gl_Position = projection * view * worldPosition;
}