#version 150
// Used by: neckercube.cpp | Object: Impossible Cube | Effect: Earth globe texture rendering


in vec3 fragPos;
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
uniform int   uFlatShade;   // 1 = flat illusion colors only (no texture/lighting/edges/animation)

out vec4 outColor;

// ─── 3D simplex noise — vendored from stegu/webgl-noise (MIT) ────────────────
// Ian McEwan, Ashima Arts; maint. Stefan Gustavson.
// https://github.com/stegu/webgl-noise — see shaders/third_party/{noise3D.glsl,LICENSE}.
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
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// 2D fbm of the simplex noise above — rock grain / micro-detail and domain warp.
float fbm2(vec2 p)
{
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; i++) { s += a * snoise(vec3(p, 0.0)); p *= 2.03; a *= 0.5; }
    return s;
}

// ─── Voronoi cell-border distance ─────────────────────────────────────────────
vec2 vhash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

// returns vec2( distanceToNearestBorder , per-cell random id )
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
    // FLAT ILLUSION MODE — solid per-face orientation color only.
    if (uFlatShade == 1) {
        outColor = vec4(fragColor.rgb, fragColor.a);
        return;
    }
    // EARLY-OUT for the ball
    if (uIsBall == 1) {
        outColor = vec4(fragColor.rgb * 2.0, 1.0);
        return;
    }

    // --- Compute face normal automatically ---
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    // --- Lighting: DIRECTIONAL light + CONSTANT view direction ---
    vec3 L = normalize(uLightPos);
    vec3 V = normalize(uEyePos);
    vec3 R = reflect(-L, N);

    // --- Phong ---
    float ambient  = 0.20;
    float diffuse  = max(dot(N, L), 0.0) * 0.75;
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.6;

    // --- Escher gradient ---
    float gradientT = clamp(vScreenY, 0.0, 1.0);
    float brightness = 0.82 + 0.20 * sin(gradientT * 3.14159 + uTime * 0.4);

    vec3 base     = fragColor.rgb;
    vec3 litColor = base * (ambient + diffuse * uLightColor) * brightness
                  + uLightColor * specular;

    // --- Edge stylization ---
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));
    float edge       = max(silhouette, crease);
    vec3  edgeColor  = vec3(0.04, 0.04, 0.07);
    litColor         = mix(litColor, edgeColor, edge);

    // ── Earthy-slot enrichment (green-gated) ──
    float earthy = clamp(uLightColor.g - max(uLightColor.r, uLightColor.b), 0.0, 1.0);

    // RIM LIGHT
    float rim = smoothstep(0.55, 0.08, facing);
    litColor += uLightColor * earthy * rim * 0.45;

    // SHEEN
    float sheen = pow(max(dot(R, V), 0.0), 120.0) * earthy * 0.9;
    litColor += uLightColor * sheen;

    // --- Solve payoff Strike ---
    vec3 strike = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0))
                * uLockGlow;
    litColor += strike;

    // ── GENERATIVE ORGANIC SOIL + MOSS GROWTH ──
    if (earthy > 0.001)
    {
        const float PI = 3.14159265;

        // Centered screen coordinate
        vec2  d     = vec2(vScreenX - 0.5, vScreenY - 0.5);

        // Position AROUND the loop
        float angle = atan(d.y, d.x) / (2.0 * PI) + 0.5;
        angle += 0.035 * sin(vScreenX * 26.0 + vScreenY * 19.0);

        // ── REVEAL TIMELINE ─────────────────────────────────────────────────
        const float EARTH_DELAY = 0.7;
        const float EARTH_TRACE = 6.0;
        float earthT       = max(uPostSolveTime - EARTH_DELAY, 0.0);
        float earthActive  = step(0.001, earthT);
        float earthFront   = earthT / EARTH_TRACE * 1.10;
        float traceW      = 0.13;
        float earthReveal  = earthActive * (1.0 - smoothstep(earthFront - traceW, earthFront + traceW, angle));

        {
            // ── 3D-ANCHORED MOSS ─────────────────────────────────────────────
            vec2  fa    = vec2(dot(dx, V), dot(dy, V));
            float flen  = length(fa);
            vec2  axis  = flen > 1e-4 ? fa / flen : vec2(1.0, 0.0);
            float slant = clamp(facing, 0.40, 1.0);

            // Spin / drift around loop
            float spin = uPostSolveTime * 0.022;
            float csR = cos(spin), snR = sin(spin);
            vec2  rd  = vec2(d.x * csR + d.y * snR,
                            -d.x * snR + d.y * csR);

            float al  = dot(rd, axis);
            float fs  = (1.0 / slant - 1.0) * 0.55;
            vec2  rp  = rd + axis * (al * fs);

            // ── GENERATIVE SOIL + MOSS ──
            vec2 q    = rp * 5.5;
            vec2 warp = vec2(fbm2(q + vec2(0.0, 0.0)),
                             fbm2(q + vec2(4.7, 1.3)));
            vec2 rc   = q + 0.55 * warp;

            vec2  vor    = voronoiBorder(rc);
            float border = vor.x;
            float cellId = vor.y;

            float grain  = 0.5 + 0.5 * fbm2(rp * 17.0);
            float grain2 = 0.5 + 0.5 * fbm2(rp * 38.0);

            float crackCore = 1.0 - smoothstep(0.0, 0.045, border);  // thin moss crack
            float crackGlow = 1.0 - smoothstep(0.0, 0.160, border);  // moss blend halo

            // ── WIND RUSTLE & BREATHING MOSS ─────────────────────────────────
            // Subtle organic wiggle of coordinates representing wind blowing moss
            vec2  mossWiggle = vec2(0.03 * sin(uTime * 1.5 + rc.x * 2.0), 0.03 * cos(uTime * 1.2 + rc.y * 2.0));
            vec2  mossCoord  = rc * 1.6 + mossWiggle;
            float mossFlow   = 0.60 + 0.40 * (0.5 + 0.5 * snoise(vec3(mossCoord, uTime * 0.10)));

            // Flow pulse (modulates color/growth sweep)
            float flowPhase = angle * 2.0 + uTime * 0.8;
            float flowPulse = 0.88 + 0.18 * sin(flowPhase);

            float wetness = (crackCore * 0.90 + crackGlow * 0.35) * mossFlow;
            float flow    = clamp(wetness * earthReveal * flowPulse, 0.0, 1.0);
            float mossM   = smoothstep(0.05, 0.34, flow);  // 0 = soil ... 1 = moss

            // ── RELIEF (organic moss bump mapping) ───────────────────────────
            float mossRipple = 0.15 * snoise(vec3(mossCoord * 6.0, uTime * 0.4));
            float h   = smoothstep(0.0, 0.22, border)
                      + 0.12 * grain + 0.06 * grain2
                      - 0.90 * crackGlow
                      + mossM * mossRipple;
            float dHx = dFdx(h);
            float dHy = dFdy(h);
            vec3  br1 = cross(dy, N);
            vec3  br2 = cross(N, dx);
            float det = dot(dx, br1);
            vec3  sgrad = sign(det) * (dHx * br1 + dHy * br2);
            vec3  Nb  = normalize(abs(det) * N - 6.5 * sgrad);

            float diffB  = max(dot(Nb, L), 0.0);
            float shadeB = clamp(0.42 + diffB * 0.78, 0.0, 1.0);
            float specB  = pow(max(dot(reflect(-L, Nb), V), 0.0), 20.0);

            // ── ALBEDO ───────────────────────────────────────────────────────
            // Rich soil brown / terracotta clay
            float tone = mix(0.78, 1.18, cellId) * (0.82 + 0.18 * grain);
            vec3  soil = mix(vec3(0.18, 0.12, 0.09),
                             vec3(0.35, 0.25, 0.20), grain) * tone;

            // Moss gradient (Deep Forest Green -> Spring Green -> Yellow Green)
            vec3  mossColor = mix(vec3(0.05, 0.22, 0.08), vec3(0.18, 0.58, 0.15),
                             smoothstep(0.00, 0.50, flow));
            mossColor = mix(mossColor, vec3(0.52, 0.85, 0.20), smoothstep(0.50, 0.88, flow));
            mossColor = mix(mossColor, vec3(0.78, 0.90, 0.32), smoothstep(0.92, 1.00, flow));

            float flowCycle = 0.5 + 0.5 * sin(flowPhase);
            vec3  flowBias  = mix(vec3(0.85, 0.95, 0.85), vec3(1.05, 1.08, 1.02), flowCycle);
            mossColor *= flowBias;

            // Forest light mottle (god rays filtering through trees)
            float forestLight = 0.88 + 0.12 * (0.5 * sin(uTime * 0.6)
                                            + 0.5 * sin(uTime * 1.7 + 1.2));

            // ── MATERIAL SPLIT ──
            vec3  mossBounce = vec3(0.08, 0.25, 0.05) * crackGlow * (1.0 - mossM)
                             * forestLight * earthActive * 0.18;
            vec3  litSoil = soil * shadeB
                          + vec3(0.50, 0.45, 0.40) * specB * (1.0 - mossM) * 0.08
                          + mossBounce;
            vec3  glow    = mossColor * (0.50 + 0.30 * flow) * forestLight;
            vec3  mossFinal = mix(litSoil, glow, mossM);

            // Re-assert edges
            mossFinal *= (1.0 - 0.55 * edge);
            litColor = mossFinal;
        }

        // ── REVEAL FRONT — vibrant golden-green bud bloom
        float lead       = exp(-pow((angle - earthFront) / (traceW * 1.2), 2.0));
        float leadActive = (1.0 - smoothstep(0.80, 1.05, earthFront)) * step(0.001, earthT);
        litColor += vec3(0.68, 0.92, 0.22) * lead * leadActive * earthy * 1.3;

        // ── AMBIENT SWEEPING LIGHT — continuous golden-green sweep
        float ambFront  = mod(earthT * 0.07, 1.0);
        float angDiff   = abs(angle - ambFront);
        angDiff         = min(angDiff, 1.0 - angDiff);
        float ambLead   = exp(-pow(angDiff / (traceW * 1.2), 2.0));
        float ambActive = smoothstep(1.0, 1.20, earthFront);
        litColor += vec3(0.68, 0.92, 0.22) * ambLead * ambActive * earthy * 1.15;
    }

    litColor  = min(litColor, vec3(1.0));
    outColor = vec4(litColor, fragColor.a);
}
