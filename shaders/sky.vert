#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    mat4 viewNoTranslation = mat4(mat3(view)); // Eliminăm translația
    gl_Position = projection * viewNoTranslation * vec4(aPos, 1.0);

    TexCoord = aTex;
}
