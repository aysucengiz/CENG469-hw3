#version 330 core
#extension GL_ARB_separate_shader_objects : enable

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them 
// here for simplicity.
uniform float exposure;
uniform float key;
uniform int currMode;
uniform float gammaCorr;
uniform int blurSize;

vec3 I = vec3(1, 1, 1);          // point light intensity
vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 kd = vec3(0.2, 1, 0.7);     // diffuse reflectance coefficient
vec3 ka = vec3(0.0, 0.0, 0.0);   // ambient reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient
vec3 lightPos = vec3(5, 5, 5);   // light position in world coordinates

uniform vec3 eyePos;


layout(location = 0)  out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;


vec4 getColor(vec2 TexCoords){
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;

    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(eyePos - FragPos);
    vec3 H = normalize(L + V);
    vec3 N = normalize(Normal);

    float NdotL = dot(N, L); // for diffuse component
    float NdotH = dot(N, H); // for specular component
    vec3 Iexp = I*exposure;
    vec3 diffuseColor = Iexp   *kd * max(0, NdotL);
    vec3 specularColor = Iexp * ks * pow(max(0, NdotH), 100);
    vec3 ambientColor = Iamb * ka;
    vec3 color = diffuseColor + specularColor + ambientColor;
    color = vec3(1.0) - exp(-color * exposure);

    return vec4(color,1);
}


void main(void)
{
	// Compute lighting. We assume lightPos and eyePos are in world
	// coordinates. fragWorldPos and fragWorldNor are the interpolated
	// coordinates by the rasterizer.
    if(currMode == 2){
        FragColor = -vec4(texture(gPosition, TexCoords).rgb, 1.0);
    }else if(currMode == 3){
        FragColor = vec4(texture(gNormal, TexCoords).rgb,1);
    }else {
        vec4 color = getColor(TexCoords);
        //float luminance = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
        color.a = 1.0;
        FragColor = color;
    }}
