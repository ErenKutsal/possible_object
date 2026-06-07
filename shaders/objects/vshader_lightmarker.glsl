#version 150

// Visible light marker — a small unlit sphere rendered AT the orbiting
// point light's world position so the user can SEE where the light is.
// Pure pass-through; the fragment shader just emits a bright color.

in vec3 vPosition;

uniform mat4 mvp;
uniform float uMarkerRadius;

out vec3 fragLocal;   // unit-sphere position, used by frag for soft falloff

void main()
{
    fragLocal = vPosition;
    gl_Position = mvp * vec4(vPosition * uMarkerRadius, 1.0);
}
