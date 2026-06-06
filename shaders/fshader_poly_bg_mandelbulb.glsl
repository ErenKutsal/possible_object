#version 150
// ─────────────────────────────────────────────────────────────────────────────
// Mandelbulb  (n=4, square slot)
//
// Power-8 Mandelbulb viewed via distance-estimation ray marching.
// Fully 3D and camera-interactive: the polygon's orbital camera orbits the
// fractal in real space, so every angle reveals a different structure.
// Deep-purple → gold orbit-trap palette.  Sparse star field in the void.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTime;
uniform float uAspect;

const float POWER   = 8.0;
const float BAILOUT = 2.5;

// ── Distance estimator + orbit-trap colour parameter ─────────────────────────
// colorT ∈ [0,1]: 0 = deep bulb interior, 1 = close to bulb boundary.
float mandelbulbDE(vec3 pos, out float colorT)
{
    vec3  z  = pos;
    float dr = 1.0;
    float r  = length(pos);
    float minR = r;

    for (int i = 0; i < 7; i++) {
        r = length(z);
        if (r > BAILOUT) break;

        minR = min(minR, r);

        // Spherical coordinates
        float theta = acos(clamp(z.z / r, -1.0, 1.0));
        float phi   = atan(z.y, z.x);
        dr = pow(r, POWER - 1.0) * POWER * dr + 1.0;

        // Power-8 Mandelbulb recurrence
        float zr = pow(r, POWER);
        theta   *= POWER;
        phi     *= POWER;
        z  = zr * vec3(sin(theta) * cos(phi),
                       sin(theta) * sin(phi),
                       cos(theta));
        z += pos;
    }

    colorT = clamp(minR * 1.4, 0.0, 1.0);
    // Log-linear DE: 0.5 * |z| * log|z| / |dz/dpos|
    return 0.5 * log(max(r, 1e-5)) * r / dr;
}

// ── Normal via central differences ───────────────────────────────────────────
vec3 mandelbulbNormal(vec3 p)
{
    const float e = 0.0018;
    float dummy;
    return normalize(vec3(
        mandelbulbDE(p + vec3(e, 0, 0), dummy) - mandelbulbDE(p - vec3(e, 0, 0), dummy),
        mandelbulbDE(p + vec3(0, e, 0), dummy) - mandelbulbDE(p - vec3(0, e, 0), dummy),
        mandelbulbDE(p + vec3(0, 0, e), dummy) - mandelbulbDE(p - vec3(0, 0, e), dummy)
    ));
}

// ── Deep-space star field ─────────────────────────────────────────────────────
float starField(vec2 uv)
{
    vec2  cell = floor(uv * 220.0);
    float h    = fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
    return step(0.996, h);
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // ── Camera ───────────────────────────────────────────────────────────────
    // Scale the orbit (camera_radius ≈ 0.5) out to viewing distance ~1.8 from
    // the Mandelbulb (which fits inside a unit sphere with power-8).
    vec3 ro = uEyePos * 3.6;
    vec3 rd = normalize(uCamForward + uCamRight * uv.x * 0.60 + uCamUp * uv.y * 0.60);

    // ── Ray march ────────────────────────────────────────────────────────────
    float t   = 0.0;
    bool  hit = false;
    float ct  = 0.0;

    for (int i = 0; i < 64; i++) {
        float d = mandelbulbDE(ro + rd * t, ct);
        if (d < 0.002) { hit = true; break; }
        if (t > 4.2)   break;
        t += max(d * 0.88, 0.001);
    }

    // ── Shade ────────────────────────────────────────────────────────────────
    vec3 col;
    if (hit) {
        vec3 p = ro + rd * t;

        // Re-evaluate for final colour parameter
        float colorT;
        mandelbulbDE(p, colorT);
        vec3 n = mandelbulbNormal(p);

        // Key light
        vec3  L    = normalize(vec3(1.4, 1.8, 1.0));
        float diff = max(dot(n, L), 0.0);
        float spec = pow(max(dot(reflect(-L, n), -rd), 0.0), 20.0);

        // Rim / Fresnel
        float rim  = pow(1.0 - abs(dot(n, rd)), 3.0);

        // Ambient occlusion approximation via step-distance
        float ao = clamp(mandelbulbDE(p + n * 0.12, colorT) * 8.0, 0.0, 1.0);

        // Deep-purple to gold palette driven by orbit trap
        vec3 matCol = mix(vec3(0.30, 0.04, 0.50),   // deep violet
                          vec3(0.92, 0.72, 0.10),   // warm gold
                          colorT);

        col  = matCol  * (0.15 + diff * 0.85) * ao;
        col += vec3(1.0, 0.95, 0.80) * spec * 0.35;
        col += vec3(0.55, 0.35, 1.00) * rim  * 0.50;

        // Light fog toward the fractal edge
        col *= exp(-t * 0.55);
    } else {
        // Deep space: near-black nebula + sparse stars
        float nebula = 0.5 + 0.5
                     * sin(uv.x * 2.8 + uTime * 0.04)
                     * sin(uv.y * 2.3 + uTime * 0.03);
        col  = vec3(0.018, 0.010, 0.055)
             + vec3(0.040, 0.005, 0.090) * nebula * (1.0 - length(uv) * 0.5);
        col += vec3(0.80, 0.90, 1.00) * starField(uv * 0.5) * 0.75;
    }

    // Vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 0.95;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
