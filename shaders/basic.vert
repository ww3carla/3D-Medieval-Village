#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

out vec3 fragPosEye;
out vec3 normalEye;
out vec2 fragTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 posEye = view * model * vec4(vPosition, 1.0);
    fragPosEye = posEye.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    normalEye = normalize(normalMatrix * vNormal);

    fragTexCoords = vTexCoords;

    gl_Position = projection * posEye;
}
