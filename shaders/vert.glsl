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

// out gl_PerVertex {
//     vec4 gl_Position;
// };
out vec3 fragmentColor;

void main() {
    // gl_PointSize = Position[gl_VertexID].z;
    gl_Position = projMat * viewMat * Position[gl_VertexID];

    //forward color data on to fragment shader
    fragmentColor = Color[gl_VertexID].xyz;
}