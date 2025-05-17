#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTexture;

out vec3 TexCoord;

void main(void)
{
    gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(inVertex, 1);
    gl_Position = gl_Position.xyww;
    // interpolate the texture
    TexCoord = inVertex;
}