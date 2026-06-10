#version 150
// Used by: arch.cpp | Object: Impossible Arch | Effect: Animated lava glow background


// Lava / Forge background — animated volcanic scene that fades in once
// the Impossible Arch is solved. Uses the same fullscreen quad vertex
// shader as the halo pass (vshader_halo.glsl).
//
// Layered effect:
//   1. Volcanic colour gradient (smoky dark grey at top → deep crimson at
//      the horizon → near-black char below), evoking the inside of a forge.
//   2. Ember drift — slow-moving bright orange/gold noise dots that rise
//      upward, like cinders lifting off cooling lava.
//   3. Heat shimmer column — a narrow vertical zone of brightened amber
//      centred on the figure, as though the arch is itself radiating heat.
//   4. Vignette darkens the periphery so the figure stays dominant.
//   5. uAmount blends everything in from the neutral pale-slate clear colour.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;   // pale slate base
uniform float uTime;        // wall-clock time for animation
uniform float uAmount;      // 0→1, fades in as the puzzle finishes

// ── Minimal self-contained simplex noise ────────────────────────────────────
vec3 _mod289v3(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 _mod289v4(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 _permute(vec4 x)  { return _mod289v4(((x * 34.0) + 10.0) * x); }
vec4 _tiSqrt(vec4 r)   { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v)
{
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    vec3 g  = step(x0.yzx, x0.xyz);
    vec3 l  = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    i = _mod289v3(i);
    vec4 p = _permute(_permute(_permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0)) +
        i.y + vec4(0.0, i1.y, i2.y, 1.0)) +
        i.x + vec4(0.0, i1.x, i2.x, 1.0));
    float n_ = 0.142857142857;
    vec3  ns  = n_ * D.wyz - D.xzx;
    vec4  j   = p - 49.0 * floor(p * ns.z * ns.z);
    vec4  x_  = floor(j * ns.z);
    vec4  y_  = floor(j - 7.0 * x_);
    vec4  xs  = x_ * ns.x + ns.yyyy;
    vec4  ys  = y_ * ns.x + ns.yyyy;
    vec4  hs  = 1.0 - abs(xs) - abs(ys);
    vec4  b0  = vec4(xs.xy, ys.xy);
    vec4  b1  = vec4(xs.zw, ys.zw);
    vec4  s0  = floor(b0) * 2.0 + 1.0;
    vec4  s1  = floor(b1) * 2.0 + 1.0;
    vec4  sh  = -step(hs, vec4(0.0));
    vec4  a0  = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4  a1  = b1.xzyw + s1.xzyw * sh.zzww;
    vec3  p0  = vec3(a0.xy, hs.x);
    vec3  p1  = vec3(a0.zw, hs.y);
    vec3  p2  = vec3(a1.xy, hs.z);
    vec3  p3  = vec3(a1.zw, hs.w);
    vec4  nm  = _tiSqrt(vec4(dot(p0,p0),dot(p1,p1),dot(p2,p2),dot(p3,p3)));
    p0 *= nm.x; p1 *= nm.y; p2 *= nm.z; p3 *= nm.w;
    vec4 m = max(0.5 - vec4(dot(x0,x0),dot(x1,x1),dot(x2,x2),dot(x3,x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m*m, vec4(dot(p0,x0),dot(p1,x1),dot(p2,x2),dot(p3,x3)));
}

void main()
{
    vec3 finalColor = uBaseColor;

    if (uAmount > 0.001)
    {
        // 1. Volcanic gradient
        float y01 = vNdc.y * 0.5 + 0.5;   // 0=bottom … 1=top

        vec3 smokeTop  = vec3(0.12, 0.08, 0.07);   // dark smoke crown
        vec3 glowHoriz = vec3(0.38, 0.09, 0.02);   // deep crimson horizon band
        vec3 charFloor = vec3(0.04, 0.02, 0.01);   // near-black cooled char

        vec3 volGrad;
        if (y01 > 0.5)
            volGrad = mix(glowHoriz, smokeTop,  (y01 - 0.5) * 2.0);
        else
            volGrad = mix(charFloor, glowHoriz,  y01 * 2.0);

        // 2. Rising embers — noise sample ascends over time (−uTime on Y axis).
        // Two layers at different speeds / scales give a chaotic ember cloud.
        vec3 uvE1 = vec3(vNdc.x * 2.0, vNdc.y * 2.2 - uTime * 0.20, uTime * 0.08);
        vec3 uvE2 = vec3(vNdc.x * 3.8 + 1.7, vNdc.y * 3.5 - uTime * 0.32, uTime * 0.11);

        float e1 = snoise(uvE1);
        float e2 = snoise(uvE2);

        // Embers only in the bright noise lobe, sharp falloff.
        float ember = pow(clamp(e1 * 0.5 + e2 * 0.5 + 0.18, 0.0, 1.0), 3.2);

        vec3 emberColor = mix(vec3(0.80, 0.18, 0.02), vec3(1.00, 0.65, 0.15), ember);
        vec3 forgeScene = volGrad + emberColor * ember * 0.55;

        // 3. Heat shimmer column — broad Gaussian centred on screen,
        //    tinted amber, evoking the column of hot air above the arch.
        float heatX  = exp(-vNdc.x * vNdc.x * 4.0);
        float breathe = 0.85 + 0.15 * sin(uTime * 1.3);
        vec3  heatTint = vec3(0.55, 0.18, 0.03) * heatX * breathe * 0.30;
        forgeScene += heatTint;

        // 4. Vignette
        float vig = 1.0 - dot(vNdc, vNdc) * 0.30;
        forgeScene *= vig;

        // 5. Blend
        finalColor = mix(uBaseColor, forgeScene, uAmount);
    }

    outColor = vec4(finalColor, 1.0);
}
