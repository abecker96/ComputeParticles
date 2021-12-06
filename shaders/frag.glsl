#version 430 core
//FRAGMENT SHADER
// About as simple as it gets.

in vec3 fragmentColor;
out vec3 color;

void main() {
    color = fragmentColor;
}