#version 330 core
// match this #version line to your other shaders if they differ (e.g. 150)

in vec4 vPosition;
in vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fN;      // world-space normal
out vec3 fP;      // world-space position

void main()
{
    vec4 worldPos = model * vPosition;
    fP = worldPos.xyz;

    // The sphere is centered at the origin in object space,
    // so the object-space normal is just the normalized position.
    vec3 objNormal = normalize(vPosition.xyz);
    fN = normalize(mat3(model) * objNormal);

    gl_Position = projection * view * worldPos;
}