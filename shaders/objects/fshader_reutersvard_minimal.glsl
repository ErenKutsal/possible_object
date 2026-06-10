#version 150

in vec3 fragPos;
in vec4 fragColor;

uniform vec3 uLightPos;
uniform vec3 uEyePos;
uniform float uTime;

uniform int uIsBall;
uniform vec3 uBallColor;
uniform int  uFlatShade;   // 1 = flat illusion colors only (no texture/lighting/edges/animation)

out vec4 outColor;

void main()
{
    // FLAT ILLUSION MODE — solid per-face orientation color only.
    if (uFlatShade == 1) {
        outColor = vec4(fragColor.rgb, fragColor.a);
        return;
    }
    // Emissive mode for the tracer ball
    if (uIsBall == 1) {
        outColor = vec4(uBallColor * 2.5, 1.0);
        return;
    }

    // Flat shading normal calculation
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    // Modern mathematical lighting (Key, Fill, Rim)
    vec3 L_key  = normalize(vec3(0.4, 1.0, 0.5));
    vec3 L_fill = normalize(vec3(-0.6, -0.2, -0.4));
    vec3 V = normalize(uEyePos);

    float diffuse_key  = max(dot(N, L_key), 0.0) * 0.70;
    float diffuse_fill = max(dot(N, L_fill), 0.0) * 0.25;
    float ambient = 0.20;

    // Use model's vertex colors (palette shades)
    vec3 baseColor = fragColor.rgb;

    vec3 litColor = baseColor * (ambient + diffuse_key + diffuse_fill);

    // High contrast mathematical edges
    float facing = abs(dot(N, V));
    float silhouette = smoothstep(0.28, 0.06, facing);
    float crease = smoothstep(0.18, 0.75, length(fwidth(N)));
    float edge = max(silhouette, crease);

    vec3 edgeColor = vec3(0.15, 0.15, 0.18);
    litColor = mix(litColor, edgeColor, edge * 0.9);

    // Subtle edge rim light to emphasize mathematical geometry
    float rim = pow(1.0 - facing, 4.0) * 0.15;
    litColor += vec3(1.0) * rim;

    outColor = vec4(litColor, 1.0);
}
