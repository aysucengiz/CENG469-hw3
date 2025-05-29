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


void main()
{
    vec4 vel = imageLoad(velocity_buffer, int(gl_GlobalInvocationID.x));
    vec4 pos = imageLoad(position_buffer, int(gl_GlobalInvocationID.x));
    //pos.xy = vec2(50.0,100.0);
    int i;
    pos.xy += vec2(1.0,10.0) * dt;
    pos.z -= 0.0001 * dt;

    for (i=0; i<currAttractor; i++){
        vec2 dist = (attractor[i].xy - pos.xy);
        vel.xy += dt * dt * attractor[i].z * normalize(dist) / (dot(dist,dist) + 10.0);
    }

    if(pos.z <= 0.0){
        pos.xy = - pos.xy * 0.01;
        vel.xy *=0.01;
        pos.z += 1.0f;
    }

    imageStore(position_buffer, int(gl_GlobalInvocationID.x), pos);
    imageStore(velocity_buffer, int(gl_GlobalInvocationID.x), vel);

}