#version 150
// Used by: arch_proc.cpp | Object: Curved Impossible Arch | Effect: Horizon skybox vertex shader


// Skybox vertex shader. Draws a unit cube whose vertex positions ARE the
// direction vectors we sample the procedural sky with.
//
// Tricks used:
//   - View matrix has its translation stripped (mat3 view) so the skybox
//     stays centered on the camera — you can never "walk to" the horizon.
//   - The output gl_Position has its z forced to w so depth = 1.0 (far
//     plane) regardless of clip-space z. The skybox always loses the depth
//     test against any rendered geometry → draws BEHIND everything.
//
// uSkyboxRotation drives a Y-axis spin so the post-solve animation rotates
// the environment around the chrome figure.

in vec3 vPosition;

uniform mat4  view;
uniform mat4  projection;
uniform float uSkyboxRotation;   // radians, animated CPU-side after solve

out vec3 fragDir;

void main()
{
    float c = cos(uSkyboxRotation);
    float s = sin(uSkyboxRotation);
    // Y-axis rotation, column-major (column 0 is x→ axis, etc.)
    mat3 yrot = mat3(vec3( c, 0.0,  s),
                     vec3( 0.0, 1.0, 0.0),
                     vec3(-s, 0.0,  c));
    vec3 rotated = yrot * vPosition;

    // Direction we're looking at — fragment shader uses this to sample sky.
    fragDir = rotated;

    // Strip translation: skybox follows camera at the origin.
    mat3 viewRot = mat3(view);
    vec4 pos = projection * mat4(viewRot) * vec4(vPosition, 1.0);
    // Force depth = 1.0 (far plane) by setting z = w.
    gl_Position = pos.xyww;
}
