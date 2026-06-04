#version 150

in vec4 vPosition;
in vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;     // world-space position for the flat-normal derivative
out vec4 fragColor;   // pass through original vertex color as base
out float vScreenY;   // screen-space (NDC) height 0=bottom..1=top, for the gradient

void main()
{
    vec4 worldPos = model * vPosition;
    fragPos       = worldPos.xyz;
    fragColor     = vColor;
    gl_Position   = projection * view * worldPos;

    // Screen-space height of this vertex. Under the orthographic projection
    // these figures use, w == 1, so this interpolates linearly and is identical
    // for any two faces that land on the same screen pixel — which is what keeps
    // a solved join seamless (the depth gradient can't step across the seam).
    vScreenY = gl_Position.y / gl_Position.w * 0.5 + 0.5;
}