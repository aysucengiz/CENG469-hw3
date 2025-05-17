#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 worldPos = modelingMatrix * vec4(aPos,1.0);
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;
    Normal = transpose(inverse(mat3x3(modelingMatrix))) * aNormal;
    gl_Position = projectionMatrix * viewingMatrix * worldPos;
}