#version 330 core

out vec4 FragColor;
  
in vec2 TexCoords;
uniform float Lw;
uniform float key;
uniform int currMode;
uniform float gammaCorr;

uniform sampler2D gTexture;
uniform int blurSize;


vec4 toneMapping(vec2 TexCoords){
    vec4 colour = vec4(texture(gTexture, TexCoords).rgb, 1.0);
    if(currMode == 0){
        colour = key * colour / Lw;
    colour = pow(colour, vec4(1.0/gammaCorr));
    }
    return colour;

}

vec4 blur(int blurSize)
{
    vec2 texelSize = 1.0 / textureSize(gTexture, 0);
    vec4 sum = vec4(0.0);
    int size = blurSize * 2 + 1;

    for (int y = -blurSize; y <= blurSize; ++y) {
        for (int x = -blurSize; x <= blurSize; ++x) {
            vec2 offset = vec2(x, y)* texelSize;
            sum += toneMapping( TexCoords + offset);
        }
    }

    return sum / float(size * size);
}


void main(void)
{
    if(blurSize > 0) FragColor = blur(blurSize);
    else             FragColor = toneMapping(TexCoords);

}
