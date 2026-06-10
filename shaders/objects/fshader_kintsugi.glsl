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
uniform int   uFlatShade;   // 1 = flat illusion colors only (no texture/lighting/edges/animation)

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

// ─── Voronoi ────────────────────────────────────────────────────────────────
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
    // FLAT ILLUSION MODE — solid per-face orientation color only.
    if (uFlatShade == 1) {
        outColor = vec4(fragColor.rgb, fragColor.a);
        return;
    }
    if (uIsBall == 1) {
        outColor = vec4(fragColor.rgb * 2.0, 1.0);
        return;
    }

    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));
    vec3 L  = normalize(uLightPos);
    vec3 V  = normalize(uEyePos);
    vec3 R  = reflect(-L, N);

    // ── Dark Obsidian Base Lighting ─────────────────────────────────────────
    float ambient  = 0.15;
    float diffuse  = max(dot(N, L), 0.0) * 0.7;
    // Obsidian has tight, bright specular highlights
    float specular = pow(max(dot(R, V), 0.0), 128.0) * 1.2;

    vec3 base = fragColor.rgb; // provided by stair_palette (we will tweak it in C++ or override here)
    // Actually, let's override the base color to a dark obsidian stone to ensure it looks right
    // base = vec3(0.08, 0.08, 0.09); // dark grey-black
    
    vec3 litColor = base * (ambient + diffuse * uLightColor) + uLightColor * specular;

    // Edge stylization
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));
    float edge       = max(silhouette, crease);
    litColor = mix(litColor, vec3(0.02, 0.02, 0.03), edge); // dark edges

    // Solve payoff strike
    vec3 strike = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0)) * uLockGlow;
    litColor += strike;

    // ── Kintsugi / Liquid Gold Effect ────────────────────────────────────────
    // We want the gold to flow through procedural cracks originating from the center.
    
    const float PI = 3.14159265;
    vec2 d = vec2(vScreenX - 0.5, vScreenY - 0.5);
    float dist = length(d); // distance from center

    // Timeline
    const float KINTSUGI_DELAY = 0.5;
    const float KINTSUGI_TRACE = 5.0;
    float goldT = max(uPostSolveTime - KINTSUGI_DELAY, 0.0);
    float goldActive = step(0.001, goldT);
    
    // Front expands radially but with noise modulation so it looks like it's flowing organically
    float noiseOffset = snoise(vec3(d * 4.0, 0.0)) * 0.15;
    float goldFront = (goldT / KINTSUGI_TRACE) * 1.0 + noiseOffset;
    
    // Smooth reveal
    float goldReveal = goldActive * smoothstep(goldFront, goldFront - 0.1, dist);

    if (goldActive > 0.0)
    {
        // 3D anchored coords
        vec2 fa = vec2(dot(dx, V), dot(dy, V));
        float flen = length(fa);
        vec2 axis = flen > 1e-4 ? fa / flen : vec2(1.0, 0.0);
        float slant = clamp(facing, 0.40, 1.0);

        vec2 rp = d * 1.0;
        float fs = (1.0 / slant - 1.0) * 0.55;
        float al = dot(rp, axis);
        rp += axis * (al * fs);

        // Crack generation
        vec2 q = rp * 6.0;
        vec2 warp = vec2(fbm2(q), fbm2(q + vec2(5.2, 1.3)));
        vec2 rc = q + 0.4 * warp; // warped coords for organic cracks

        vec2 vor = voronoiBorder(rc);
        float border = vor.x;
        float cellId = vor.y;

        // Sharp crack definition
        float crackCore = 1.0 - smoothstep(0.0, 0.03, border);
        float crackGlow = 1.0 - smoothstep(0.0, 0.12, border);

        // Gold flow: it only fills the cracks where goldReveal is active
        float goldMask = clamp((crackCore * 1.2 + crackGlow * 0.3) * goldReveal, 0.0, 1.0);

        // Base gold material (metallic, warm, highly reflective)
        vec3 goldAlbedo = vec3(1.00, 0.75, 0.20);
        
        // Flowing effect: bright pulses moving along the cracks
        float flowPulse = 0.5 + 0.5 * sin(dist * 20.0 - uTime * 4.0 + cellId * PI);
        vec3 goldEmission = mix(vec3(1.0, 0.6, 0.1), vec3(1.0, 0.9, 0.4), flowPulse);

        // Bump mapping for the stone and cracks
        float grain = fbm2(rp * 20.0);
        float h = 0.05 * grain - 0.5 * crackGlow * (1.0 - goldMask) + 0.2 * goldMask;
        float dHx = dFdx(h);
        float dHy = dFdy(h);
        vec3 br1 = cross(dy, N);
        vec3 br2 = cross(N, dx);
        float det = dot(dx, br1);
        vec3 sgrad = sign(det) * (dHx * br1 + dHy * br2);
        vec3 Nb = normalize(abs(det) * N - 4.0 * sgrad);

        float diffB = max(dot(Nb, L), 0.0);
        float specB = pow(max(dot(reflect(-L, Nb), V), 0.0), goldMask > 0.5 ? 96.0 : 64.0);

        // Recompute lighting with the bumped normal
        vec3 litObsidian = base * (ambient + diffB * uLightColor) + uLightColor * specB * 1.2;
        litObsidian *= mix(0.7, 1.1, cellId); // subtle block variation

        // Add subtle golden rim reflection on the obsidian when gold is active
        float goldReflect = pow(clamp(1.0 - dot(Nb, V), 0.0, 1.0), 3.0) * goldReveal;
        litObsidian += vec3(0.5, 0.3, 0.05) * goldReflect * 0.8;

        vec3 litGold = goldAlbedo * (0.2 + diffB) + uLightColor * specB * 2.0 + goldEmission * 1.5;

        // Blend obsidian and gold
        vec3 combined = mix(litObsidian, litGold, goldMask);

        // Re-apply edge
        combined = mix(combined, vec3(0.02, 0.01, 0.0), edge);

        litColor = combined;
    }

    litColor = min(litColor, vec3(1.0));
    outColor = vec4(litColor, fragColor.a);
}
