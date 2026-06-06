#version 150

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

    // ── Cool-slot enrichment (water/cyan-gated) ──
    float coolness = clamp(uLightColor.b - uLightColor.r, 0.0, 1.0);

    // RIM LIGHT
    float rim = smoothstep(0.55, 0.08, facing);
    litColor += uLightColor * coolness * rim * 0.45;

    // SHEEN
    float sheen = pow(max(dot(R, V), 0.0), 120.0) * coolness * 0.9;
    litColor += uLightColor * sheen;

    // --- Solve payoff Strike ---
    vec3 strike = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0))
                * uLockGlow;
    litColor += strike;

    // ── GENERATIVE WET STONE + FLOWING WATER ──
    if (coolness > 0.001)
    {
        const float PI = 3.14159265;

        // Centered screen coordinate
        vec2  d     = vec2(vScreenX - 0.5, vScreenY - 0.5);

        // Position AROUND the loop
        float angle = atan(d.y, d.x) / (2.0 * PI) + 0.5;
        angle += 0.035 * sin(vScreenX * 26.0 + vScreenY * 19.0);

        // ── REVEAL TIMELINE ─────────────────────────────────────────────────
        const float WATER_DELAY = 0.7;
        const float WATER_TRACE = 6.0;
        float waterT       = max(uPostSolveTime - WATER_DELAY, 0.0);
        float waterActive  = step(0.001, waterT);
        float waterFront   = waterT / WATER_TRACE * 1.10;
        float traceW      = 0.13;
        float waterReveal  = waterActive * (1.0 - smoothstep(waterFront - traceW, waterFront + traceW, angle));

        {
            // ── 3D-ANCHORED FLOW ─────────────────────────────────────────────
            vec2  fa    = vec2(dot(dx, V), dot(dy, V));
            float flen  = length(fa);
            vec2  axis  = flen > 1e-4 ? fa / flen : vec2(1.0, 0.0);
            float slant = clamp(facing, 0.40, 1.0);

            // SPIN CCW around loop
            float spin = uPostSolveTime * 0.022;
            float csR = cos(spin), snR = sin(spin);
            vec2  rd  = vec2(d.x * csR + d.y * snR,
                            -d.x * snR + d.y * csR);

            float al  = dot(rd, axis);
            float fs  = (1.0 / slant - 1.0) * 0.55;
            vec2  rp  = rd + axis * (al * fs);

            // ── GENERATIVE WET STONE + FLOWING WATER (procedural, seam-safe) ──
            vec2 q    = rp * 5.5;
            vec2 warp = vec2(fbm2(q + vec2(0.0, 0.0)),
                             fbm2(q + vec2(4.7, 1.3)));
            vec2 rc   = q + 0.55 * warp;

            vec2  vor    = voronoiBorder(rc);
            float border = vor.x;
            float cellId = vor.y;

            float grain  = 0.5 + 0.5 * fbm2(rp * 17.0);
            float grain2 = 0.5 + 0.5 * fbm2(rp * 38.0);

            float crackCore = 1.0 - smoothstep(0.0, 0.045, border);  // thin water stream
            float crackGlow = 1.0 - smoothstep(0.0, 0.160, border);  // water halo/wetness

            // ── WATER ADVECTION ──────────────────────────────────────────────
            float rr        = length(d);
            vec2  waterTan   = rr > 1e-4 ? vec2(-d.y, d.x) / rr : vec2(0.0);
            vec2  waterCoord = rc * 1.8 - waterTan * (uTime * 0.18);
            float waterFlow  = 0.60 + 0.40 * (0.5 + 0.5 * snoise(vec3(waterCoord, uTime * 0.15)));

            // FLOW PULSE
            float flowPhase = angle * 2.0 + uTime * 1.5;
            float flowPulse = 0.88 + 0.18 * sin(flowPhase);

            float wetness = (crackCore * 0.90 + crackGlow * 0.35) * waterFlow;
            float flow    = clamp(wetness * waterReveal * flowPulse, 0.0, 1.0);
            float waterM  = smoothstep(0.05, 0.34, flow);  // 0 = slate ... 1 = water stream

            // ── RELIEF (procedural normal map, Mikkelsen surface gradient) ───
            float waterRipple = 0.15 * sin(waterCoord.x * 4.0 + uTime * 2.0) * cos(waterCoord.y * 4.0 + uTime * 2.0);
            float h   = smoothstep(0.0, 0.22, border)
                      + 0.12 * grain + 0.06 * grain2
                      - 0.90 * crackGlow
                      + waterM * waterRipple;
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
            // Dark blue-grey wet slate
            float tone = mix(0.78, 1.18, cellId) * (0.82 + 0.18 * grain);
            vec3  stone = mix(vec3(0.08, 0.11, 0.14),
                             vec3(0.22, 0.28, 0.35), grain) * tone;

            // Water gradient
            vec3  waterColor = mix(vec3(0.02, 0.15, 0.42), vec3(0.05, 0.52, 0.92),
                             smoothstep(0.00, 0.50, flow));
            waterColor = mix(waterColor, vec3(0.18, 0.78, 1.00), smoothstep(0.50, 0.82, flow));
            waterColor = mix(waterColor, vec3(0.42, 0.95, 1.00), smoothstep(0.88, 1.00, flow));

            float flowCycle = 0.5 + 0.5 * sin(flowPhase);
            vec3  flowBias  = mix(vec3(0.86, 0.92, 0.98), vec3(1.02, 1.05, 1.08), flowCycle);
            waterColor *= flowBias;

            // Caustic shimmering
            float caustics = 0.85 + 0.15 * (0.5 * sin(uTime * 1.6)
                                         + 0.5 * sin(uTime * 3.7 + 2.0));

            // ── MATERIAL SPLIT ──
            vec3  coolBounce = vec3(0.05, 0.18, 0.35) * crackGlow * (1.0 - waterM)
                             * caustics * waterActive * 0.18;
            vec3  litStone = stone * shadeB
                          + vec3(0.60, 0.78, 0.90) * specB * (1.0 - waterM) * 0.10
                          + coolBounce;
            vec3  glow    = waterColor * (0.50 + 0.30 * flow) * caustics;
            vec3  waterFinal = mix(litStone, glow, waterM);

            // Re-assert edges
            waterFinal *= (1.0 - 0.55 * edge);
            litColor = waterFinal;
        }

        // ── REVEAL FRONT — cyan lead bloom
        float lead       = exp(-pow((angle - waterFront) / (traceW * 1.2), 2.0));
        float leadActive = (1.0 - smoothstep(0.80, 1.05, waterFront)) * step(0.001, waterT);
        litColor += vec3(0.20, 0.85, 1.0) * lead * leadActive * coolness * 1.3;

        // ── AMBIENT SWEEPING LIGHT — continuous cool sweep
        float ambFront  = mod(waterT * 0.07, 1.0);
        float angDiff   = abs(angle - ambFront);
        angDiff         = min(angDiff, 1.0 - angDiff);
        float ambLead   = exp(-pow(angDiff / (traceW * 1.2), 2.0));
        float ambActive = smoothstep(1.0, 1.20, waterFront);
        litColor += vec3(0.20, 0.85, 1.0) * ambLead * ambActive * coolness * 1.15;
    }

    litColor  = min(litColor, vec3(1.0));
    outColor = vec4(litColor, fragColor.a);
}
