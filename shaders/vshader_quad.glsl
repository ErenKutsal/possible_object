#version 330 core

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vTexCoord;

out vec2 fragUV;

void main() {
    fragUV = vTexCoord;
    gl_Position = vec4(vPosition, 0.0, 1.0);
}