#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

layout( std430, binding=4 ) buffer Pos
{   vec4 Positions[];  };  // array of structures
layout( std430, binding=5 ) buffer Vel
{   vec4 Velocities[]; };  // array of structures
layout( std430, binding=6 ) buffer Col
{   vec4 Colors[];  };  // array of structures

layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;
// uniform vec3 blackHole1;
// uniform vec3 blackHole2;

const vec4 Sphere1 = vec4( 0.0, -50.0, 0.0, 48.50 ); // x, y, z, r
const vec4 Sphere2 = vec4( 0.0, -50.0, 0.0, 50.50);
// (could also have passed this in)
vec3 Bounce( vec3 vin, vec3 n )
{
    // const float F = 0.70;
    vec3 vout = reflect( vin, n );
    return vout;
}
vec3 BounceSphere( vec3 p, vec3 v, vec4 s )
{
    vec3 n = normalize( p - s.xyz );
    return Bounce( v, n );
}
bool IsInsideSphere( vec3 p, vec4 s )
{
    float r = length( p - s.xyz );
    return ( r < s.w );
}

void main() {
    const vec3 G = vec3(0, -9.8, 0);
    const float DT = 0.003;
    const float AirResistance = 0.998;

    uint gid = gl_GlobalInvocationID.x;

    vec3 p = Positions[gid].xyz;
    vec3 v = Velocities[gid].xyz;

    vec3 pp = p + v*DT + 0.5*DT*DT*G;
    vec3 vp = v + v*DT + G*DT;

    if( IsInsideSphere( pp, Sphere1 ) )
    {
        vp = BounceSphere( p, v, Sphere1 );
        pp = p + vp*DT + .5*DT*DT*G;
    }
    if( !IsInsideSphere( pp, Sphere2 ) )
    {
        vp = BounceSphere( p, v, Sphere2 );
        pp = p + vp*DT + .5*DT*DT*G;
    }

    Positions[gid].xyz = pp;
    Velocities[gid].xyz = vp*AirResistance;
}