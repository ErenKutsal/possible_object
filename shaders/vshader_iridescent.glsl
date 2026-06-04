#version 150

in vec4 vPosition;
in vec4 vColor;

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;

// (uPostSolveTime / uBreathAmount are still passed by ObjShape, but slot 6
// no longer breathes — the post-solve reward is the iridescent shimmer in
// the fragment shader, not a geometry pulse. The uniforms are kept as a
// no-op so the C++ side doesn't need a special case.)
uniform float uPostSolveTime;
uniform float uBreathAmount;

out vec3 fragPos;
out vec4 fragColor;

void main()
{
    vec4 worldPos = model * vPosition;
    fragPos       = worldPos.xyz;
    fragColor     = vColor;
    gl_Position   = projection * view * worldPos;
}
