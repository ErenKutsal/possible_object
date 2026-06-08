#version 150

uniform vec3 uFaceColor;
out vec4 fColor;

void main() 
{
    fColor = vec4(uFaceColor, 1.0); 
}