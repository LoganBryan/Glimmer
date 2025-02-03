#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 texCoord;
out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
	fragPosition = vec3(model * vec4(position, 1.0));
	fragNormal = normalize(mat3(transpose(inverse(model))) * normal);
	texCoord = uv;
	gl_Position = projection * view * vec4(fragPosition, 1.0);
}