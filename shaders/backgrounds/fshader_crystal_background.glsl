#version 150

// Crystal / Electric Storm background — fades in once the Penrose Blocks
// are solved. Uses the same fullscreen-quad vertex shader (vshader_halo.glsl).
//
// Layered effect:
//   1. Deep space gradient — near-black indigo crown → dark violet mid → black floor.
//   2. Lightning bolt arcs — procedural branching bolts that spawn outward
//      from a central point and fade with distance, driven by snoise.
//   3. Electric pulse rings — concentric circles that expand outward from the
//      center, imitating the shockwave after a lightning strike.
//   4. Stardust — a faint field of tiny bright pixels (crystal fragments).
//   5. Vignette pulls focus to the figure.
//   6. uAmount blends everything in from the pale-slate neutral colour.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;
uniform float uTime;
uniform float uAmount;

// ── Simplex noise (self-contained) ──────────────────────────────────────────
vec3 _m289v3(vec3 x) { return x - floor(x * (1.0/289.0)) * 289.0; }
vec4 _m289v4(vec4 x) { return x - floor(x * (1.0/289.0)) * 289.0; }
vec4 _perm(vec4 x)   { return _m289v4(((x*34.0)+10.0)*x); }
vec4 _tiSq(vec4 r)   { return 1.79284291400159 - 0.85373472095314 * r; }

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
    i = _m289v3(i);
    vec4 p = _perm(_perm(_perm(
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
    vec4  s0  = floor(b0)*2.0+1.0;
    vec4  s1  = floor(b1)*2.0+1.0;
    vec4  sh  = -step(hs, vec4(0.0));
    vec4  a0  = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4  a1  = b1.xzyw + s1.xzyw * sh.zzww;
    vec3  p0  = vec3(a0.xy, hs.x);
    vec3  p1  = vec3(a0.zw, hs.y);
    vec3  p2  = vec3(a1.xy, hs.z);
    vec3  p3  = vec3(a1.zw, hs.w);
    vec4  nm  = _tiSq(vec4(dot(p0,p0),dot(p1,p1),dot(p2,p2),dot(p3,p3)));
    p0*=nm.x; p1*=nm.y; p2*=nm.z; p3*=nm.w;
    vec4 m = max(0.5 - vec4(dot(x0,x0),dot(x1,x1),dot(x2,x2),dot(x3,x3)), 0.0);
    m = m*m;
    return 105.0 * dot(m*m, vec4(dot(p0,x0),dot(p1,x1),dot(p2,x2),dot(p3,x3)));
}

// ── Fractal branching lightning helper ───────────────────────────────────────
// Returns a brightness value for a bolt radiating from origin along (angle).
// Uses noise to jitter the bolt path so it looks organic, not straight.
float boltLine(vec2 uv, float angle, float seed, float sharpness)
{
    float c = cos(angle), s = sin(angle);
    // Rotate uv into bolt-local frame
    vec2  local = vec2(uv.x * c + uv.y * s,
                      -uv.x * s + uv.y * c);
    // Only emit in the forward half
    if (local.x < 0.0) return 0.0;

    // Jitter the perpendicular offset with noise (creates branching kinks)
    float jitter = snoise(vec3(local.x * 3.0, seed, uTime * 0.7)) * 0.18;
    float perp   = local.y - jitter;

    // Distance from the bolt centreline, attenuated by forward distance
    float lineWidth = 0.012 / (1.0 + local.x * 2.0);
    float brightness = exp(-perp * perp / (lineWidth * lineWidth));

    // Fade with distance from center
    brightness *= exp(-local.x * 1.8);
    return clamp(brightness, 0.0, 1.0);
}

void main()
{
    vec3 finalColor = uBaseColor;

    if (uAmount > 0.001)
    {
        // 1. ── Deep space gradient ────────────────────────────────────────────
        float y01 = vNdc.y * 0.5 + 0.5;  // 0=bottom … 1=top

        vec3 deepIndigo = vec3(0.04, 0.02, 0.14);  // deep indigo crown
        vec3 midViolet  = vec3(0.08, 0.03, 0.20);  // dark violet mid
        vec3 voidBlack  = vec3(0.01, 0.00, 0.05);  // near-black void floor

        vec3 spaceGrad;
        if (y01 > 0.5)
            spaceGrad = mix(midViolet, deepIndigo, (y01 - 0.5) * 2.0);
        else
            spaceGrad = mix(voidBlack, midViolet, y01 * 2.0);

        // 2. ── Lightning bolt arcs ────────────────────────────────────────────
        // Six bolts radiate outward at different angles, each slowly rotating
        // and flickering on a different period so they never all flash together.
        float boltBright = 0.0;

        // Slow overall rotation of the bolt array
        float rotT = uTime * 0.18;

        for (int b = 0; b < 6; b++)
        {
            float bAngle = float(b) * (3.14159265 / 3.0) + rotT;
            float seed   = float(b) * 7.391;
            // Each bolt flickers on its own phase
            float flicker = 0.5 + 0.5 * sin(uTime * (3.0 + float(b) * 0.7) + seed);
            flicker = pow(flicker, 2.5);  // sharper off-time
            boltBright += boltLine(vNdc, bAngle, seed, 28.0) * flicker;
        }

        // Branch bolts: 12 thinner secondary bolts between the primary ones
        for (int b = 0; b < 12; b++)
        {
            float bAngle = float(b) * (3.14159265 / 6.0) + rotT * 0.6 + 0.26;
            float seed   = float(b) * 3.713 + 100.0;
            float flicker = 0.5 + 0.5 * sin(uTime * (5.0 + float(b) * 0.4) + seed);
            flicker = pow(flicker, 3.5);
            boltBright += boltLine(vNdc, bAngle, seed, 40.0) * flicker * 0.40;
        }

        boltBright = clamp(boltBright, 0.0, 1.0);

        // Bolt colour: white-hot core bleeding to electric blue/cyan
        vec3 boltColor = mix(vec3(0.15, 0.35, 1.00), vec3(1.00, 1.00, 1.00),
                             pow(boltBright, 1.5));
        vec3 stormScene = spaceGrad + boltColor * boltBright * 1.2;

        // 3. ── Concentric pulse rings ─────────────────────────────────────────
        // Rings expand outward from center, imitating shockwaves.
        float r = length(vNdc);
        // Three rings at different speeds and opacities
        float ring1 = pow(max(0.0, sin(r * 12.0 - uTime * 4.5)), 8.0);
        float ring2 = pow(max(0.0, sin(r * 20.0 - uTime * 7.0 + 1.2)), 10.0) * 0.6;
        float ring3 = pow(max(0.0, sin(r * 7.0  - uTime * 2.8 + 2.4)), 6.0) * 0.4;
        float rings = clamp(ring1 + ring2 + ring3, 0.0, 1.0);
        // Fade rings toward screen edges so they don't overpower the figure area
        rings *= exp(-r * r * 1.8);
        stormScene += vec3(0.20, 0.50, 1.00) * rings * 0.55;

        // 4. ── Stardust / crystal fragment field ─────────────────────────────
        // Tiny sparkles across the background — high-frequency noise threshold.
        vec3 dustUV = vec3(vNdc * 60.0, uTime * 0.05);
        float dust  = snoise(dustUV);
        float sparkle = pow(clamp(dust, 0.0, 1.0), 18.0);  // only the very peaks
        stormScene += vec3(0.70, 0.80, 1.00) * sparkle * 0.9;

        // 5. ── Central glow — the "epicentre" of the strike ──────────────────
        float epicentre = exp(-r * r * 6.0);
        float breathe   = 0.80 + 0.20 * sin(uTime * 2.1);
        stormScene += vec3(0.30, 0.55, 1.00) * epicentre * breathe * 0.60;

        // 6. ── Vignette ───────────────────────────────────────────────────────
        float vig = 1.0 - dot(vNdc, vNdc) * 0.32;
        stormScene *= clamp(vig, 0.0, 1.0);

        // 7. ── Blend from neutral slate to storm ─────────────────────────────
        finalColor = mix(uBaseColor, stormScene, uAmount);
    }

    outColor = vec4(finalColor, 1.0);
}
