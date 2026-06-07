#version 150

// Emissive (unlit) marker. Outputs a bright color so the marker reads as
// "a glowing thing" against the dark skybox. Soft Fresnel-style falloff at
// the silhouette so the orb feels like it has volume / is glowing rather
// than a flat painted disc.

in vec3 fragLocal;

uniform vec3 uLightColor;
uniform vec3 uEyePosLocal;   // eye position in the marker's local frame

out vec4 outColor;

void main()
{
    vec3 N = normalize(fragLocal);
    vec3 V = normalize(uEyePosLocal);
    float NdotV = max(dot(N, V), 0.0);

    // Glow: bright at the centre (facing the camera), warmer/dimmer at
    // the edges. Mimics a bloom-y bulb without actually doing a bloom pass.
    float core   = pow(NdotV, 1.4);
    float halo   = pow(1.0 - NdotV, 1.5);

    vec3 col = uLightColor * (1.0 + 2.0 * core) + uLightColor * 0.6 * halo;
    col = clamp(col, vec3(0.0), vec3(2.5));   // allow some HDR before tonemap

    // Gamma to match the rest of the scene.
    col = col / (col + vec3(1.0));            // mild Reinhard
    col = pow(col, vec3(1.0 / 2.2));

    outColor = vec4(col, 1.0);
}
