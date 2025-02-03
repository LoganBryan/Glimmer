#version 460 core

in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPosition;

uniform sampler2D decalTexture;
uniform float decalStrength;

out vec4 fragColor;

void main()
{
	vec4 baseColor = vec4(1.0);
	vec4 decalColor = texture(decalTexture, texCoord);

	fragColor = mix(baseColor, decalColor, decalStrength);
}