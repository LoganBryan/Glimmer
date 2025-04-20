#version 460 core

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 position;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	mat4 mvm = view * model;

	normal = mat3(transpose(inverse(model))) * aNormal;
	position = (mvm * vec4(aVertex, 1.0)).xyz;
	gl_Position = projection * mvm * vec4(aVertex, 1.0);
}