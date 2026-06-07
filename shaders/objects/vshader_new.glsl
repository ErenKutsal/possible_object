#version 150

in vec3 vPosition;
in vec3 vNormal;

uniform mat4 MVP;
uniform mat4 uModel;

out vec3 fragPos;    // world-space position
out vec3 fragNormal; // world-space normal
out vec3 fragLocalNormal;

void main() {
    vec4 worldPos = uModel * vec4(vPosition, 1.0);
    fragPos    = worldPos.xyz;
    fragLocalNormal = vNormal;
    // Normal matrix corrects normals under non-uniform scale
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    fragNormal = normalize(normalMatrix * vNormal);

    gl_Position = MVP * vec4(vPosition, 1.0);
}