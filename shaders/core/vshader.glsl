#version 150

in vec3 vPosition;
uniform mat4 MVP;

void main() 
{
    // Apply our matrix to the position
    gl_Position = MVP * vec4(vPosition, 1.0);
}