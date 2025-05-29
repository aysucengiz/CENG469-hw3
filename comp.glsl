#version 430 core

layout (std140, binding = 0) uniform attractor_block{
    vec4 attractor[12]; // xy position, z mass, w padding
};

layout (local_size_x = 128) in;
layout (rgba32f, binding=0) uniform imageBuffer velocity_buffer;
layout (rgba32f, binding=1) uniform imageBuffer position_buffer;
uniform float dt;
uniform int currAttractor;
uniform float origin_x;
uniform float origin_y;
uniform int gWidth;
uniform int gHeight;
uniform float startAge;

void main()
{
    vec4 vel = imageLoad(velocity_buffer, int(gl_GlobalInvocationID.x));
    vec4 pos = imageLoad(position_buffer, int(gl_GlobalInvocationID.x));
    pos.z -= 0.1* dt;
    if(pos.z <= 0.0){
        pos.xy = vec2(origin_x,origin_y);
        vel.xy *=0.01;
        pos.z += 1.0f;
    }
    int i;
    pos.xy += vec2(1.0,10.0) * dt;

    for (i=0; i<currAttractor; i++){
        vec2 dist = (attractor[i].xy - pos.xy);
        vel.xy += dt * dt * attractor[i].z * normalize(dist) / (dot(dist,dist) + 10.0);
    }

    if((pos.x > gWidth/2.0) || (pos.x < -gWidth/2.0)){
        pos.x = -pos.x;
    }

    if((pos.y > gHeight/2.0) || (pos.y <  -gHeight/2.0)){
        pos.y = -pos.y;
    }

    imageStore(position_buffer, int(gl_GlobalInvocationID.x), pos);
    imageStore(velocity_buffer, int(gl_GlobalInvocationID.x), vel);

}