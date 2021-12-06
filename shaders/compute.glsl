#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

//Compute shader
// Uses a Position, Velocity, and Color SSBO as input and output
layout( std430, binding=4 ) buffer Pos
{   vec4 Positions[];  };
layout( std430, binding=5 ) buffer Vel
{   vec4 Velocities[]; };
layout( std430, binding=6 ) buffer Col
{   vec4 Colors[];  };

// local work group is 100 large. I believe ideal local size would be GCD(num_cores, num_particles)
layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;

// uniform control variables
uniform int sphereEnable;
uniform int floorEnable;
uniform float blackHoleAccel;
uniform float DT;
uniform float colorScale;
uniform float floorPos;
uniform vec3 blackHole1;    //xyz position
uniform vec3 blackHole2;
uniform vec3 startColor;
uniform vec3 endColor;
uniform vec4 sphere;        //xyz position, w radius

// Function just checks if a position is inside of a sphere or not
bool IsInsideSphere( vec3 p, vec4 s )
{
    float r = length( p - s.xyz );
    return ( r < s.w );
}

// Calculates acceleration towards a position
// uses uniform G(m*M) instead of calculating every frame
vec3 accelTowardsBH(vec3 pPos, vec3 bhPos){
    vec3 dir = bhPos - pPos;
    float r = length(dir);

    //norm(direction) * (G*m*M)/(r*r)
    // divideBy(r*r*r) avoids an unnecessary division
    vec3 accelVec = blackHoleAccel*dir/(r*r*r);
    return accelVec;
}

void main() {
    // used in color picking
    const float e = 2.7182818284;

    // gid used as index into SSBO to find the particle
    // that any particular instance is controlling
    uint gid = gl_GlobalInvocationID.x;

    // Get position and velocity of this particle
    vec3 p = Positions[gid].xyz;
    vec3 v = Velocities[gid].xyz;

    // Update acceleration towards both masses
    vec3 accelVec = accelTowardsBH(p, blackHole1);
    accelVec += accelTowardsBH(p, blackHole2);

    // Use Verlet Integration for physics modeling
    vec3 pp = p + v*DT + 0.5*DT*DT*accelVec*(sign(DT));
    vec3 vp = v + accelVec*DT;

    if( sphereEnable == 1 && !IsInsideSphere( pp, sphere ) )
    {
        // If new point is outside of the sphere, and the sphere should exist
        // Set new point to be very close to the edge of the sphere
        //  in the direction where it would have hit
        pp = (normalize(pp) * (sphere.w-1.0)) + sphere.xyz;
        vp = vec3(0, 0, 0);
    }
    if( floorEnable == 1 && pp.y < floorPos)
    {
        // Currently only support major axis planes, and one where the normal
        //  is in the +Z direction at that
        // If the point would have gone below the floor, set new position to be
        //  right above the floor, and reflect the new velocity over the normal
        pp.y = floorPos+1.0;
        vp = reflect(vp, vec3(0, 1, 0));
    }

    // scale between two colors based on particle velocity
    float lengthVP = length(vp);
    // (1-e^(-kx)) used here
    float scale = (1-pow(e, -lengthVP*colorScale));

    // subtract out the initial (low-speed) color
    vec3 outColor = startColor - startColor*scale;
    // as we add in the final (high-speed) color
    outColor += endColor*scale;

    // Update new position, velocity, and color in SSBO for rendering
    Positions[gid].xyz = pp;
    Velocities[gid].xyz = vp;
    Colors[gid] = vec4(outColor, 1.0);
}