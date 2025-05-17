#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform samplerCube mySampler;
uniform float exposure;
uniform float key;
uniform int currMode;
uniform float gammaCorr;
uniform int blurSize;
in vec3 TexCoord;

layout(location = 0)  out vec4 fragColor;



void main(void)
{
    vec3 hdrColor = texture(mySampler, TexCoord).rgb;
    vec3 mapped = hdrColor;
    mapped = vec3(1.0) - exp(-mapped * exposure);

    float alpha = 0.2126*mapped.r + 0.7152*mapped.g + 0.0722*mapped.b;
    fragColor = vec4(mapped, alpha);
}