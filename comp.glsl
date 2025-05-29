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

    pos.z -= 0.01* dt;
    pos.xy += vel.xy* dt;
    int i;
    for (i=0; i<currAttractor; i++){
        vec2 dist = vec2((attractor[i].x / gWidth) - (pos.x / gWidth), (attractor[i].y / gHeight) - (pos.y / gHeight));
        vel.xy += dt * normalize(dist) * attractor[i].z/ (dot(dist,dist)+10);
    }

    if(pos.z <= 0.0){
        pos.xy = vec2(origin_x,origin_y);
        vel.xy = vec2(vel.z,vel.w);
        pos.z = 1.0f;
    }
    if((pos.x > gWidth) || (pos.x < 0.0)){
        pos.x = -pos.x;
    }

    if((pos.y > gHeight) || (pos.y <  0.0)){
        pos.y = -pos.y;
    }

    imageStore(position_buffer, int(gl_GlobalInvocationID.x), pos);
    imageStore(velocity_buffer, int(gl_GlobalInvocationID.x), vel);

}