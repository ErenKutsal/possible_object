#version 150
// Used by: polygon_bg.cpp | Object: Impossible Polygon (slot 0, bg index 3) | Effect: Menger Sponge fractal background

// ─────────────────────────────────────────────────────────────────────────────
// Menger Sponge  (n=6+, hexagon slot and beyond)
//
// A 3-D infinite Menger sponge ray-marched with a correct iterative SDF.
// Camera-aware: the polygon's orbital camera (eye/right/up/forward) drives the
// view, so dragging the polygon camera also orbits around the fractal.
// Hot magenta / deep violet palette — thematically paired with the
// cyan+magenta impossible-polygon bars at this slot.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTime;
uniform float uAspect;

const int   MAX_STEPS = 128;
const float MAX_DIST  = 30.0;
const float SURF_DIST = 0.001;

// ── Rotation helpers ─────────────────────────────────────────────────────────
mat2 rot2(float a) { float c = cos(a), s = sin(a); return mat2(c,-s, s, c); }

// ── Menger Sponge SDF (standard iterative fold) ──────────────────────────────
// Reference: Inigo Quilez / Syntopia "Menger Sponge" IFS
// The sponge lives in [-1,1]^3.  We start with a unit box and carve out
// cross-shaped tunnels at every scale with 4 iterations.
float sdBox3(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q, vec3(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// One iteration of the cross removal at a given scale
float crossSDF(vec3 p)
{
    // infinite cross: min of three axis-aligned square-prisms
    float da = max(abs(p.x), abs(p.y));
    float db = max(abs(p.y), abs(p.z));
    float dc = max(abs(p.x), abs(p.z));
    return min(da, min(db, dc)) - (1.0 / 3.0);
}

float sdMenger(vec3 p)
{
    float d = sdBox3(p, vec3(1.0));

    float s = 1.0;
    for (int i = 0; i < 4; i++) {
        // Fold into [−1,1]^3 cell at scale s, then replicate with period 2/s
        vec3 a = mod(p * s, 2.0) - 1.0;
        s *= 3.0;
        vec3 r = 1.0 - 3.0 * abs(a);
        // cross SDF of the carved tunnels at this level
        float c = (min(max(r.x, r.y), min(max(r.y, r.z), max(r.x, r.z)))) / s;
        d = max(d, c);
    }
    return d;
}

float sceneSDF(vec3 p)
{
    // Gentle continuous rotation so the sponge never looks static
    p.xz = rot2(uTime * 0.09) * p.xz;
    p.xy = rot2(uTime * 0.05) * p.xy;
    return sdMenger(p);
}

vec3 calcNormal(vec3 p)
{
    const float e = 0.001;
    return normalize(vec3(
        sceneSDF(p + vec3(e, 0.0, 0.0)) - sceneSDF(p - vec3(e, 0.0, 0.0)),
        sceneSDF(p + vec3(0.0, e, 0.0)) - sceneSDF(p - vec3(0.0, e, 0.0)),
        sceneSDF(p + vec3(0.0, 0.0, e)) - sceneSDF(p - vec3(0.0, 0.0, e))
    ));
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Place the camera outside the unit sponge, driven by the polygon camera.
    // The polygon uses camera_radius ≈ 0.5 so uEyePos is short — normalise it
    // and pull it out to r=3.5 so we orbit clearly around the sponge.
    vec3 ro = normalize(uEyePos + vec3(0.001, 0.0, 0.0)) * 3.5;

    // Build ray from the polygon's orbital basis vectors
    vec3 rd = normalize(uCamForward * 1.8 + uCamRight * uv.x + uCamUp * uv.y);

    // ── Ray march ──
    float t   = 0.1;   // start slightly ahead to avoid self-intersection at ro
    bool  hit = false;
    for (int i = 0; i < MAX_STEPS; i++) {
        float d = sceneSDF(ro + rd * t);
        if (d < SURF_DIST * t) { hit = true; break; }
        if (t > MAX_DIST) break;
        t += max(d, SURF_DIST);
    }

    vec3 col;

    if (hit) {
        vec3 p = ro + rd * t;
        vec3 n = calcNormal(p);

        // ── Hot magenta / violet palette ──────────────────────────────────────
        // Lambertian from a warm key light
        vec3 keyDir = normalize(vec3(0.8, 1.2, 0.6));
        float diff  = max(dot(n, keyDir), 0.0);

        vec3 shadowCol = vec3(0.06, 0.01, 0.18);  // deep indigo-violet
        vec3 litCol    = vec3(1.00, 0.15, 0.75);  // hot magenta
        col = mix(shadowCol, litCol, pow(diff, 0.7));

        // Rim light in electric cyan
        float rim = pow(clamp(1.0 - dot(n, -rd), 0.0, 1.0), 3.5);
        col += vec3(0.05, 0.85, 1.0) * rim * 0.6;

        // Specular
        vec3 refl = reflect(rd, n);
        float spec = pow(max(dot(refl, keyDir), 0.0), 48.0);
        col += vec3(1.0, 0.55, 0.90) * spec * 0.45;

        // Depth fog — sponge fades into the dark void
        float fog = exp(-t * 0.15);
        col = mix(vec3(0.02, 0.0, 0.08), col, fog);
    }
    else
    {
        // Deep space void with a faint violet glow toward the sponge
        float glare = pow(max(dot(rd, -normalize(uCamForward)), 0.0), 8.0);
        col = vec3(0.02, 0.00, 0.08) + vec3(0.30, 0.04, 0.45) * glare * 0.4;
    }

    // Vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 1.1;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
