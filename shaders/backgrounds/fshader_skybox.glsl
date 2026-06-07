#version 150

// Skybox fragment shader. Samples the same baked procedural cubemap that
// the chrome figure samples for its reflection — guarantees the skybox and
// the reflections show identical content.

in vec3 fragDir;
out vec4 outColor;

uniform samplerCube uEnvMap;

void main()
{
    // Sample at mip 0 (sharpest) — we want the skybox crisp.
    vec3 col = textureLod(uEnvMap, normalize(fragDir), 0.0).rgb;
    // Gamma to match the chrome shader's tonemapped output.
    col = pow(col, vec3(1.0 / 2.2));
    outColor = vec4(col, 1.0);
}
