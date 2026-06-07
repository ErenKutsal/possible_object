#version 150

// Water background — animated deep ocean/river scene that fades in
// once the Reutersvard Rectangle is solved. Uses the same fullscreen
// quad vertex shader as the halo pass (vshader_halo.glsl).
//
// Layered effect:
//   1. Deep-water colour gradient (dark navy at top → teal at centre → dark abyss below).
//   2. Caustic shimmer — simplex noise-based bright ripple patches that
//      drift across the frame, imitating light refracted through a water surface.
//   3. Slow horizontal caustic sweeps to reinforce the sense of underwater current.
//   4. Vignette pulls focus to the figure at the centre.
//   5. uAmount blends the whole thing in from the neutral pale-slate clear colour.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;   // pale slate base (matches main.cpp clear colour)
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
        // 1. Deep ocean gradient — dark navy crown, deep teal/cyan mid, dark abyss at bottom.
        vec3 deepOcean  = vec3(0.00, 0.04, 0.14);   // navy crown
        vec3 midOcean   = vec3(0.01, 0.16, 0.32);   // dark teal midwater
        vec3 abyssColor = vec3(0.00, 0.02, 0.08);   // near-black abyss

        float y01 = vNdc.y * 0.5 + 0.5;  // 0=bottom … 1=top
        vec3 oceanGrad;
        if (y01 > 0.5)
            oceanGrad = mix(midOcean, deepOcean,  (y01 - 0.5) * 2.0);
        else
            oceanGrad = mix(abyssColor, midOcean, y01 * 2.0);

        // 2. Caustic ripple patches — slow horizontal drift.
        // Two noise layers at different scales and drift speeds give an
        // organic, non-repeating caustic feel.
        vec3 uvA = vec3(vNdc * 1.6 + vec2(uTime * 0.07, -uTime * 0.04), uTime * 0.10);
        vec3 uvB = vec3(vNdc * 2.8 + vec2(-uTime * 0.05, uTime * 0.03), uTime * 0.13 + 4.7);

        float cA = snoise(uvA);
        float cB = snoise(uvB);

        // Bright caustic patches only in the positive noise lobe.
        float caustic = pow(clamp(cA * 0.55 + cB * 0.45 + 0.25, 0.0, 1.0), 2.8);

        // Caustic colour: ice-white core bleeding to cyan.
        vec3 causticColor = mix(vec3(0.05, 0.50, 0.80), vec3(0.70, 0.95, 1.00), caustic);
        vec3 oceanScene = oceanGrad + causticColor * caustic * 0.38;

        // 3. Slow horizontal bands — long-wavelength ripple adds depth.
        float band = 0.5 + 0.5 * sin(vNdc.y * 6.0 + uTime * 0.5);
        oceanScene = mix(oceanScene, oceanScene * vec3(0.85, 0.98, 1.05), band * 0.08);

        // 4. Vignette — darkens corners, draws eye to the figure.
        float vig = 1.0 - dot(vNdc, vNdc) * 0.28;
        oceanScene *= vig;

        // 5. Blend from the neutral base to the water scene.
        finalColor = mix(uBaseColor, oceanScene, uAmount);
    }

    outColor = vec4(finalColor, 1.0);
}
