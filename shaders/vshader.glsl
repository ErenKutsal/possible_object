#version 150
in vec3 vPosition;
in vec3 vColor;

out vec4 color;

uniform mat4 MVP;

void main() 
{
    // Apply our matrix to the position
    gl_Position = MVP * vec4(vPosition, 1.0);
    color = vec4(vColor, 1.0);
}