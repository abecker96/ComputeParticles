#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

layout( std430, binding=4 ) buffer Pos
{   vec4 Positions[];  };  // array of structures
layout( std430, binding=5 ) buffer Vel
{   vec4 Velocities[]; };  // array of structures
layout( std430, binding=6 ) buffer Col
{   vec4 Colors[];  };  // array of structures

layout( local_size_x = 1000, local_size_y = 1, local_size_z = 1 ) in;



void main() {
    const vec3 G = vec3(0, -9.8, 0);
    const float DT = 0.003;

    uint gid = gl_GlobalInvocationID.x;

    vec3 p = Positions[gid].xyz;
    vec3 v = Velocities[gid].xyz;

    vec3 pp = p + v*DT + 0.5*DT*DT*G;
    vec3 vp = v + v*DT + G*DT;

    Positions[gid].xyz = pp;
    Velocities[gid].xyz = vp;
}