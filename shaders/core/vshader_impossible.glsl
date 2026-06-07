#version 150

in vec4 vPosition;
in vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;     // world-space position for the flat-normal derivative
out vec3 vModelPos;   // model-space position — used to LOCK the rock texture
                      //   to the figure's surface. fragPos changes as the
                      //   user rotates the object; vModelPos does not, so
                      //   sampling the rock coordinate from it makes the
                      //   pattern stay glued to each surface point.
out vec4 fragColor;   // pass through original vertex color as base
out float vScreenY;   // screen-space (NDC) height 0=bottom..1=top, for the gradient
out float vScreenX;   // screen-space (NDC) width  0=left..1=right (burn-front trace)

void main()
{
    vec4 worldPos = model * vPosition;
    fragPos       = worldPos.xyz;
    vModelPos     = vPosition.xyz;
    fragColor     = vColor;
    gl_Position   = projection * view * worldPos;

    // Screen-space height of this vertex. Under the orthographic projection
    // these figures use, w == 1, so this interpolates linearly and is identical
    // for any two faces that land on the same screen pixel — which is what keeps
    // a solved join seamless (the depth gradient can't step across the seam).
    vScreenY = gl_Position.y / gl_Position.w * 0.5 + 0.5;
    vScreenX = gl_Position.x / gl_Position.w * 0.5 + 0.5;
}