#version 430 core
//VERTEX SHADER

//layout location needs to match attribute in glVertexAttribPointer()
// "in" notes that this is input data
// takes a vec3, vertexPosition_modelspace is just a name that makes sense
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
    gl_Position = projMat * viewMat * Position[gl_VertexID];
    gl_PointSize = particleSizeScalar/gl_Position.z;
    // gl_PointSize = 2.0;

    //forward color data on to fragment shader
    fragmentColor = Color[gl_VertexID].xyz;
}