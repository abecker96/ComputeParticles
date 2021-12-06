#version 430 core
//VERTEX SHADER

// Position, Velocity, Color SSBOs
// Direct from the compute shader
layout( std430, binding=4 ) buffer Pos
{   vec4 Position[];  };  // array of structures
layout( std430, binding=5 ) buffer Vel
{   vec4 Velocity[];  };  // array of structures
layout( std430, binding=6 ) buffer Col
{   vec4 Color[];  };  // array of structures

uniform mat4 viewMat;
uniform mat4 projMat;
uniform float particleSizeScalar;

out vec3 fragmentColor;

void main() {
    // Set position accoring to VP matrices
    gl_Position = projMat * viewMat * Position[gl_VertexID];
    // Modify particle size to give a sense of depth
    // A realistic model would divide by (z*z), but there are only so many pixels in a screen
    gl_PointSize = particleSizeScalar/gl_Position.z;

    //forward color data on to fragment shader
    fragmentColor = Color[gl_VertexID].xyz;
}