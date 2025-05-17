#version 330 core

uniform samplerCube mySampler;

in vec3 TexCoord;

out vec4 fragColor;

void main(void)
{
    vec3 hdrColor = texture(mySampler, TexCoord).rgb;
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    mapped = pow(mapped, vec3(1.0/2.2));
    fragColor = vec4(mapped, 1.0);
}
