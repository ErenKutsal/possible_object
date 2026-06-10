#version 150

// Dedicated shader for the Penrose Triangle (continuous variant).
// Renders the figure as metallic chrome steel (PBR with custom skybox reflection)
// and displays a glowing holographic cyber grid and iridescent ball-excitation trail.

in vec3 fragPos;
in vec3 vModelPos;   // model-space position (rock texture coord — rotation-invariant)
in vec4 fragColor;
in float vScreenY;
in float vScreenX;

uniform vec3  uLightPos;
uniform vec3  uLightColor;   // per-slot light tint (Tier 2); white = neutral
uniform vec3  uEyePos;
uniform float uTime;        // for animating the gradient
uniform float uObjHeight;   // world-space height of the object (for normalizing Y)
uniform float uLockGlow;    // 0..1, brightness pulse when figure clicks into solved pose
uniform float uPostSolveTime; // seconds since the figure LOCKED (0 while unsolved)
uniform int   uIsBall;      // 1 = render as bright emissive ball, 0 = normal figure
uniform vec3  uBallWorldPos; // world-space ball centre (zero when no ball active)
uniform float uBallRadius;   // ball world-space radius (0 = no ball — skips reflection)

uniform int uFlatShade;   // 1 = flat illusion colors only (no texture/lighting/edges/animation)
out vec4 outColor;

void main()
{
    // FLAT ILLUSION MODE — solid per-face orientation color only.
    if (uFlatShade == 1) {
        outColor = vec4(fragColor.rgb, fragColor.a);
        return;
    }
    // EARLY-OUT for the ball: bright emissive yellow, no shading, so it
    // reads as a "you-solved-it" indicator riding the figure rather than
    // a 3D object in the scene.
    if (uIsBall == 1) {
        outColor = vec4(fragColor.rgb * 2.0, 1.0);
        return;
    }

    // --- Compute face normal automatically from position derivatives ---
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    // --- Lighting: DIRECTIONAL light + CONSTANT view direction ---
    vec3 L = normalize(uLightPos);
    vec3 V = normalize(uEyePos);
    vec3 R = reflect(-L, N);

    // --- Escher gradient ---
    float gradientT = clamp(vScreenY, 0.0, 1.0);
    float brightness = 0.82 + 0.20 * sin(gradientT * 3.14159 + uTime * 0.4);

    vec3 base = fragColor.rgb;

    // --- Edge stylization ---
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));
    float edge       = max(silhouette, crease);
    vec3  edgeColor  = vec3(0.04, 0.04, 0.07);

    // ── METALLIC CHROME — full PBR override ─────────────────────────────────
    float NdotV  = max(dot(N, V), 0.0);
    float NdotL  = max(dot(N, L), 0.0);

    // Schlick-Fresnel: metals use their albedo as F0 (coloured specular).
    vec3  F0      = mix(base, vec3(0.88, 0.90, 0.96), 0.55);
    vec3  fresnel = F0 + (vec3(1.0) - F0) * pow(1.0 - NdotV, 5.0);

    // Tight specular lobe (high-power Blinn-Phong approximating GGX).
    vec3  Hk    = normalize(L + V);
    float NdotHk = max(dot(N, Hk), 0.0);
    float spec2  = pow(NdotHk, 220.0) * 2.5;

    // Near-zero diffuse — real metals have no subsurface scatter.
    float diff2  = NdotL * 0.06;

    // Key-light contribution.
    vec3  metalLit = (base * diff2 + fresnel * spec2) * uLightColor * brightness;

    // Fake environment map: sky-gradient sampled along the reflected direction.
    vec3  reflDir = reflect(-V, N);
    float envT    = clamp(reflDir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3  envLow  = vec3(0.05, 0.06, 0.10);
    vec3  envMid  = vec3(0.50, 0.56, 0.70);
    vec3  envHigh = vec3(0.90, 0.94, 1.00);
    vec3  envCol  = mix(envLow, envMid,  smoothstep(0.0, 0.5, envT));
    envCol        = mix(envCol, envHigh, smoothstep(0.5, 1.0, envT));
    // Metals tint their reflections with F0.
    vec3  envRefl = envCol * F0 * (0.35 + 0.65 * fresnel.r);

    vec3 litColor = metalLit + envRefl;

    // Re-apply edge lines so bars stay readable.
    litColor = mix(litColor, edgeColor, edge);

    // ── Ball proximity and metallic reflection ────────────────────────────
    // The chrome ball casts a bright specular glint onto each bar as it passes
    // close — a clean metallic white highlight (no iridescent grid overlay).
    if (uBallRadius > 0.0) {
        float bDist  = length(uBallWorldPos - fragPos);
        vec3  Lball  = normalize(uBallWorldPos - fragPos + vec3(1e-5));
        vec3  Hball  = normalize(Lball + V);
        float bSpec  = pow(max(dot(N, Hball), 0.0), 200.0);
        float bAtten = 1.0 / (1.0 + bDist * bDist * 0.30);
        litColor += vec3(0.95, 0.97, 1.00) * bSpec * bAtten * 3.5;
    }

    // Lock-glow pulse still applies.
    vec3 strikeM = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0)) * uLockGlow;
    litColor += strikeM;

    litColor = min(litColor, vec3(1.0));
    outColor = vec4(litColor, fragColor.a);
}
