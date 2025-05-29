#version 430 core


layout (location = 0) out vec4 color;
in float age;
in vec2 loc;

void main()
{
    //color = mix(vec4(1.0f,0.0f,0.0f,1.0f),vec4(0.0f,0.0f,0.1f,1.0f),age);
    color = vec4(loc,0.0f,1.0f);
}