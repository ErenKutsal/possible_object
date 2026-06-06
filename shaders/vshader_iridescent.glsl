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
out float vScreenX;   // screen-space (NDC) x 0=left..1=right — for the temper clock-trace
out float vScreenY;   // screen-space (NDC) y 0=bottom..1=top

void main()
{
    vec4 worldPos = model * vPosition;
    fragPos       = worldPos.xyz;
    fragColor     = vColor;
    gl_Position   = projection * view * worldPos;

    // Seam-safe screen position: under the orthographic projection w==1, so
    // these interpolate linearly and match for any two faces landing on the
    // same pixel — keeps the temper sweep continuous across the magic join.
    vScreenX = gl_Position.x / gl_Position.w * 0.5 + 0.5;
    vScreenY = gl_Position.y / gl_Position.w * 0.5 + 0.5;
}
