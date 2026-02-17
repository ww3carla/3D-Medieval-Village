#version 410 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D diffuseTexture;

void main()
{
    FragColor = texture(diffuseTexture, vec2(TexCoord.x, 1.0 - TexCoord.y));
}
