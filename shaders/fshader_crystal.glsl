#version 150

in vec3 fragPos;
in vec4 fragColor;
in float vScreenY;
in float vScreenX;

uniform vec3  uLightPos;
uniform vec3  uLightColor;
uniform vec3  uEyePos;
uniform float uTime;
uniform float uObjHeight;
uniform float uLockGlow;
uniform float uPostSolveTime;
uniform int   uIsBall;

out vec4 outColor;

// ─── 3D simplex noise ────────────────────────────────────────────────────────
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+10.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v)
{
    const vec2  C = vec2(1.0/6.0, 1.0/3.0);
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 =   v - i + dot(i, C.xxx);
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    i = mod289(i);
    vec4 p = permute(permute(permute(
               i.z + vec4(0.0, i1.z, i2.z, 1.0))
             + i.y + vec4(0.0, i1.y, i2.y, 1.0))
             + i.x + vec4(0.0, i1.x, i2.x, 1.0));
    float n_ = 0.142857142857;
    vec3  ns = n_ * D.wyz - D.xzx;
    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;
    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
    vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

float fbm2(vec2 p)
{
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; i++) { s += a * snoise(vec3(p, 0.0)); p *= 2.03; a *= 0.5; }
    return s;
}

// ─── Voronoi (same as water/earth) ───────────────────────────────────────────
vec2 vhash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

vec2 voronoiBorder(vec2 x)
{
    vec2 n = floor(x);
    vec2 f = fract(x);
    vec2 mr = vec2(0.0);
    vec2 mg = vec2(0.0);
    float md = 8.0;
    for (int j = -1; j <= 1; j++)
    for (int i = -1; i <= 1; i++)
    {
        vec2 g = vec2(float(i), float(j));
        vec2 o = vhash2(n + g);
        vec2 r = g + o - f;
        float d = dot(r, r);
        if (d < md) { md = d; mr = r; mg = g; }
    }
    float border = 8.0;
    for (int j = -2; j <= 2; j++)
    for (int i = -2; i <= 2; i++)
    {
        vec2 g = mg + vec2(float(i), float(j));
        vec2 o = vhash2(n + g);
        vec2 r = g + o - f;
        vec2 dr = r - mr;
        if (dot(dr, dr) > 1e-5)
            border = min(border, dot(0.5 * (mr + r), normalize(dr)));
    }
    float cellId = fract(sin(dot(n + mg, vec2(12.9898, 78.233))) * 43758.5453);
    return vec2(border, cellId);
}

void main()
{
    if (uIsBall == 1) {
        outColor = vec4(fragColor.rgb * 2.0, 1.0);
        return;
    }

    // ── Geometry ────────────────────────────────────────────────────────────
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));
    vec3 L  = normalize(uLightPos);
    vec3 V  = normalize(uEyePos);
    vec3 R  = reflect(-L, N);

    // ── Base Phong ──────────────────────────────────────────────────────────
    float ambient  = 0.20;
    float diffuse  = max(dot(N, L), 0.0) * 0.75;
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.6;
    float gradientT = clamp(vScreenY, 0.0, 1.0);
    float brightness = 0.82 + 0.20 * sin(gradientT * 3.14159 + uTime * 0.4);

    vec3 base     = fragColor.rgb;
    vec3 litColor = base * (ambient + diffuse * uLightColor) * brightness
                  + uLightColor * specular;

    // ── Edge stylization ────────────────────────────────────────────────────
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));
    float edge       = max(silhouette, crease);
    litColor = mix(litColor, vec3(0.04, 0.04, 0.07), edge);

    // ── "Electric" gate — triggers on blue-dominant light ──────────────────
    // penrose_blocks sets uLightColor with strong blue, so this gate is 1.
    float electric = clamp(uLightColor.b - max(uLightColor.r, uLightColor.g) * 0.7, 0.0, 1.0);

    // ── Solve payoff strike ─────────────────────────────────────────────────
    vec3 strike = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0)) * uLockGlow;
    litColor += strike;

    // ── CRYSTAL LIGHTNING — RADIAL SHATTER ─────────────────────────────────
    // Unlike the CCW loop sweep in water/earth, this animation expands
    // RADIALLY from the geometric center of the screen. The reveal front is
    // a growing circle (radius = CRYSTAL_FRONT), not an angular sweep.
    // Each Voronoi cell lights up when the front reaches it, and then
    // pulses with electric arcs that oscillate in frequency over time.
    if (electric > 0.001)
    {
        const float PI = 3.14159265;

        // Centered screen coord in [-0.5, 0.5]
        vec2 d = vec2(vScreenX - 0.5, vScreenY - 0.5);
        float dist = length(d);   // radial distance from center [0, ~0.7]

        // ── REVEAL TIMELINE (radial, not angular) ──────────────────────────
        // CRYSTAL_DELAY: brief pause before the shatter begins
        // CRYSTAL_TRACE: seconds for the front to expand from center to edge
        const float CRYSTAL_DELAY = 0.4;
        const float CRYSTAL_TRACE = 5.0;
        float crystalT      = max(uPostSolveTime - CRYSTAL_DELAY, 0.0);
        float crystalActive = step(0.001, crystalT);
        // Front radius: 0→0.85 over CRYSTAL_TRACE seconds (0.85 covers full screen)
        float crystalFront  = crystalT / CRYSTAL_TRACE * 0.85;
        float frontW        = 0.10;
        // A pixel is REVEALED if its radial distance is LESS than the front
        float crystalReveal = crystalActive * (1.0 - smoothstep(crystalFront - frontW,
                                                                  crystalFront + frontW, dist));

        {
            // ── 3D-ANCHORED CRYSTAL FACETS ─────────────────────────────────
            vec2  fa   = vec2(dot(dx, V), dot(dy, V));
            float flen = length(fa);
            vec2  axis = flen > 1e-4 ? fa / flen : vec2(1.0, 0.0);
            float slant = clamp(facing, 0.40, 1.0);

            // Crystal lattice — NO rotation over time (blocks are static geometry)
            // Instead the UV slowly scales (breathing / pulsing facets)
            float breathe  = 1.0 + 0.04 * sin(uTime * 1.8);
            vec2  rp       = d * breathe;
            float fs       = (1.0 / slant - 1.0) * 0.55;
            float al       = dot(rp, axis);
            rp += axis * (al * fs);

            // Crystal voronoi — smaller cells than water/earth (higher frequency = more facets)
            vec2 q    = rp * 8.0;
            vec2 warp = vec2(fbm2(q + vec2(0.0, 0.0)),
                             fbm2(q + vec2(3.3, 5.7)));
            vec2 rc   = q + 0.30 * warp;   // less warping than earth — facets stay geometric

            vec2  vor    = voronoiBorder(rc);
            float border = vor.x;
            float cellId = vor.y;

            float grain  = 0.5 + 0.5 * fbm2(rp * 14.0);

            // Crack = thin crystal edge. Glow = halo of electric charge around edge.
            float crackCore = 1.0 - smoothstep(0.0, 0.025, border);  // sharp electric edge
            float crackGlow = 1.0 - smoothstep(0.0, 0.120, border);  // energy halo

            // ── ELECTRIC ARC PULSE ─────────────────────────────────────────
            // High-frequency oscillation that increases in speed as the
            // animation progresses — the lightning "builds up" before settling.
            float arcSpeed  = 4.0 + crystalT * 1.5;
            float arcCoord  = dist * 8.0 + uTime * arcSpeed;
            float arcPulse  = 0.5 + 0.5 * sin(arcCoord);
            // A per-cell offset so each facet fires slightly independently
            float cellPhase = cellId * PI * 6.28;
            float cellArc   = 0.5 + 0.5 * sin(arcCoord + cellPhase);
            float arc       = mix(arcPulse, cellArc, 0.5);

            // Flow: how much "charge" a pixel carries (crack + arc)
            float charge = (crackCore * 1.0 + crackGlow * 0.45) * arc;
            float flow   = clamp(charge * crystalReveal, 0.0, 1.0);
            float crystalM = smoothstep(0.04, 0.40, flow);  // 0=dark mineral, 1=electric arc

            // ── RELIEF — flat facet planes with sharp crystal edges ─────────
            // Much less organic than moss — the height gradient is based on
            // the voronoi border (flat cells separated by sharp ridges).
            float h   = smoothstep(0.0, 0.18, border)
                      + 0.08 * grain
                      - 0.85 * crackGlow;
            float dHx = dFdx(h);
            float dHy = dFdy(h);
            vec3  br1 = cross(dy, N);
            vec3  br2 = cross(N, dx);
            float det = dot(dx, br1);
            vec3  sgrad = sign(det) * (dHx * br1 + dHy * br2);
            vec3  Nb  = normalize(abs(det) * N - 5.5 * sgrad);

            float diffB  = max(dot(Nb, L), 0.0);
            float shadeB = clamp(0.35 + diffB * 0.80, 0.0, 1.0);
            float specB  = pow(max(dot(reflect(-L, Nb), V), 0.0), 48.0);

            // ── ALBEDO ─────────────────────────────────────────────────────
            // Dark mineral base (anthracite / dark blue-grey flint)
            float tone = mix(0.75, 1.20, cellId) * (0.85 + 0.15 * grain);
            vec3  mineral = mix(vec3(0.06, 0.08, 0.14),
                                vec3(0.14, 0.18, 0.28), grain) * tone;

            // Electric arc color:
            // Deep charge → icy white-blue core → violet arc → cyan afterglow
            vec3  arcColor = mix(vec3(0.04, 0.06, 0.22), vec3(0.30, 0.50, 1.00),
                            smoothstep(0.00, 0.40, flow));
            arcColor = mix(arcColor, vec3(0.75, 0.85, 1.00), smoothstep(0.40, 0.72, flow));
            arcColor = mix(arcColor, vec3(1.00, 1.00, 1.00), smoothstep(0.80, 1.00, flow));

            // Subtle afterglow color flicker driven by cellId + time
            float flickerT = sin(uTime * 3.5 + cellId * 12.0) * 0.5 + 0.5;
            vec3  flickerTint = mix(vec3(0.60, 0.50, 1.00), vec3(0.30, 0.80, 1.00), flickerT);
            arcColor = mix(arcColor, flickerTint, crystalM * 0.25);

            // ── MATERIAL SPLIT ──────────────────────────────────────────────
            vec3  chargeAura = vec3(0.05, 0.12, 0.40) * crackGlow * (1.0 - crystalM)
                             * crystalActive * arc * 0.22;
            vec3  litMineral = mineral * shadeB
                             + vec3(0.50, 0.60, 1.00) * specB * (1.0 - crystalM) * 0.18
                             + chargeAura;
            vec3  glow = arcColor * (0.55 + 0.35 * flow) * (0.9 + 0.1 * arc);
            vec3  crystalFinal = mix(litMineral, glow, crystalM);

            // Re-assert edges
            crystalFinal *= (1.0 - 0.50 * edge);
            litColor = crystalFinal;
        }

        // ── REVEAL FRONT — white-hot lightning strike ring ──────────────────
        // A sharp Gaussian ring centred on the expanding front radius.
        float ringDist = abs(dist - crystalFront);
        float lead     = exp(-pow(ringDist / (frontW * 0.8), 2.0));
        float leadActive = (1.0 - smoothstep(0.70, 0.90, crystalFront))
                         * step(0.001, crystalT);
        // White-hot centre + cyan fringe
        vec3 leadColor = mix(vec3(0.50, 0.80, 1.00), vec3(1.00, 1.00, 1.00),
                             clamp(lead * 2.0, 0.0, 1.0));
        litColor += leadColor * lead * leadActive * electric * 1.6;

        // ── RESONANCE RIPPLE — after full coverage, periodic pulse rings ────
        // Once crystalFront >= 0.85 (fully covered), concentric rings pulse
        // from center outward — like after-shocks from the initial strike.
        float settled = smoothstep(0.70, 0.90, crystalFront);
        float ringPhase = dist * 14.0 - uTime * 5.0;
        float ripple = pow(max(0.0, sin(ringPhase)), 6.0);
        litColor += vec3(0.20, 0.55, 1.00) * ripple * settled * electric * 0.55;
    }

    litColor  = min(litColor, vec3(1.0));
    outColor = vec4(litColor, fragColor.a);
}
