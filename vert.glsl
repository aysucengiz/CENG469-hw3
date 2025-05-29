#version 430 core

layout (location = 0) in vec4 aPos;

uniform mat4 projectionMatrix;
uniform float particleSize;


out float age;
out vec2 loc;

void main()
{
    gl_Position = projectionMatrix* vec4(aPos.x, aPos.y, 0.0 , 1.0);
    age = aPos.z;
    loc = aPos.xy;
    gl_PointSize = particleSize;
}