#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

layout( std430, binding=4 ) buffer Pos
{   vec4 Positions[];  };
layout( std430, binding=5 ) buffer Vel
{   vec4 Velocities[]; };
layout( std430, binding=6 ) buffer Col
{   vec4 Colors[];  };

layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;

uniform int sphereEnable;
uniform int floorEnable;
uniform float blackHoleAccel;
uniform float DT;
uniform float colorScale;
uniform float floorPos;
uniform vec3 blackHole1;
uniform vec3 blackHole2;
uniform vec3 startColor;
uniform vec3 endColor;
uniform vec4 sphere;

// (could also have passed this in)
vec3 Bounce( vec3 vin, vec3 n )
{
    const float F = 0.0;
    vec3 vout = reflect( vin, n );
    return vout*F;
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

vec3 accelTowardsBH(vec3 pPos, vec3 bhPos){
    vec3 dir = bhPos - pPos;
    float r = length(dir);

    vec3 accelVec = blackHoleAccel*dir/(r*r*r);
    return accelVec;
}

void main() {
    // const vec3 G = vec3(0, -9.8, 0);
    // const float DT = 0.0003;
    const float AirResistance = 0.1;
    const float e = 2.7182818284;

    uint gid = gl_GlobalInvocationID.x;

    vec3 p = Positions[gid].xyz;
    vec3 v = Velocities[gid].xyz;

    vec3 accelVec = accelTowardsBH(p, blackHole1);
    accelVec += accelTowardsBH(p, blackHole2);

    vec3 pp = p + v*DT + 0.5*DT*DT*accelVec*(sign(DT));
    vec3 vp = v + accelVec*DT;

    if( sphereEnable == 1 && !IsInsideSphere( pp, sphere ) )
    {
        pp = (normalize(p) * (sphere.w-1.0)) + sphere.xyz;
        vp = vec3(0, 0, 0);
    }
    if( floorEnable == 1 && pp.y < floorPos)
    {
        pp.y = floorPos+1.0;
        vp = reflect(vp, vec3(0, 1, 0));
    }

    float lengthVP = length(vp);
    float scale = (1-pow(e, -lengthVP*colorScale));

    vec3 outColor = startColor - startColor*scale;
    outColor += endColor*scale;

    Positions[gid].xyz = pp;
    Velocities[gid].xyz = vp;
    Colors[gid] = vec4(outColor, 1.0);
}