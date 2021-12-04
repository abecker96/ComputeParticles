#version 430 core
//FRAGMENT SHADER

in vec3 fragmentColor;
out vec3 color;

void main() {
    color = fragmentColor;
}