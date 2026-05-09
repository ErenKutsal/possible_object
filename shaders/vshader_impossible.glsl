#version 150

in vec4 vPosition;
in vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;     // world-space position for Phong + gradient
out vec4 fragColor;   // pass through original vertex color as base

void main()
{
    vec4 worldPos = model * vPosition;
    fragPos       = worldPos.xyz;
    fragColor     = vColor;
    gl_Position   = projection * view * worldPos;
}