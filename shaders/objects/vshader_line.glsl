#version 150

in vec4 vPosition; // xyz = coordinate, w = cumulative arc length

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float fArcLength;

void main()
{
    gl_Position = projection * view * model * vec4(vPosition.xyz, 1.0);
    fArcLength = vPosition.w;
}
