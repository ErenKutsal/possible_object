#version 150
// Used by: polygon_bg.cpp | Object: Impossible Polygon (slot 0, bg index 0) | Effect: Escher corridor/gallery background

// ─────────────────────────────────────────────────────────────────────────────
// Escher Corridor  (n=3, triangle slot)  — FIXED
//
// Infinite network of cross-shaped tunnels viewed from inside.
// KEY FIX: ray origin is anchored at (2,2,z) — the centre of the nearest
// tunnel cell in mod-4 space — plus a small eye-position parallax offset.
// This guarantees we always start inside open tunnel space regardless of
// where the polygon's orbital camera is.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTime;
uniform float uAspect;

// ── Helpers ─────────────────────────────────────────────────────────────────
mat2 rot2(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

// Inside-out cross-tunnel SDF.
// Convention: positive  = inside open air (far from wall),
//             zero      = at the wall surface,
//             negative  = inside solid (outside the tunnel network).
// A ray marching from inside steps toward 0 and hits the wall.
float mapInside(vec3 p)
{
    // Slow twist along Z so the corridor feels like it spirals inward
    float twist = p.z * 0.07 + uTime * 0.035;
    p.xy = rot2(twist) * p.xy;

    // Modular repeat every 4 units
    vec3 q = mod(p, vec3(4.0)) - 2.0;

    // Three axis-aligned square-tunnel interiors (positive = inside the tube)
    float tZ = 1.10 - max(abs(q.x), abs(q.y));
    float tX = 1.10 - max(abs(q.y), abs(q.z));
    float tY = 1.10 - max(abs(q.x), abs(q.z));

    // Union: inside the network if inside ANY of the three tunnels.
    // max(tZ, tX, tY) gives the largest "remaining air gap" from any wall.
    return max(tZ, max(tX, tY));
}

// Finite-difference surface normal (gradient points toward tunnel centre)
vec3 calcNormal(vec3 p)
{
    const float e = 0.003;
    return normalize(vec3(
        mapInside(p + vec3(e, 0, 0)) - mapInside(p - vec3(e, 0, 0)),
        mapInside(p + vec3(0, e, 0)) - mapInside(p - vec3(0, e, 0)),
        mapInside(p + vec3(0, 0, e)) - mapInside(p - vec3(0, 0, e))
    ));
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // ── Ray origin: always inside a tunnel cell ──────────────────────────────
    // The cross-tunnels are centred at (2,2,z) in mod-4 space.
    // uEyePos (orbit radius ~0.5) is used as a *small* parallax shift so the
    // background rotates with the polygon camera, but its magnitude (×0.28)
    // keeps us comfortably inside the 1.10-unit tunnel half-width.
    vec3 ro = vec3(2.0, 2.0, 0.0)
            + uEyePos * 0.28
            + vec3(0.0, 0.0, uTime * 0.40);  // drift forward

    // Camera-aware ray direction from the polygon's orbital camera
    vec3 rd = normalize(uCamForward + uCamRight * uv.x * 0.72 + uCamUp * uv.y * 0.72);

    // ── Ray march ────────────────────────────────────────────────────────────
    float t   = 0.0;
    bool  hit = false;
    for (int i = 0; i < 100; i++) {
        float d = mapInside(ro + rd * t);

        // Hit only when we are still inside (d>=0) and approaching a wall
        if (d >= 0.0 && d < 0.004) { hit = true; break; }
        if (t > 52.0) break;

        // abs(d): safe to step whether we are inside (d>0) or clipped into
        // solid (d<0) — the abs brings us back to open air either way.
        t += max(abs(d) * 0.82, 0.018);
    }

    // ── Shading ──────────────────────────────────────────────────────────────
    vec3 col = vec3(0.0);
    if (hit) {
        vec3 p  = ro + rd * t;
        vec3 n  = calcNormal(p);

        // Flip normal so it faces the viewer (the gradient points inward,
        // which is toward the viewer — correct for inside-surface shading).
        // No flip needed: positive direction IS toward tunnel centre = viewer.
        float diff = max(dot(n, normalize(vec3(0.3, 0.9, 0.5))), 0.0) * 0.65 + 0.30;
        float fog  = exp(-t * 0.052);

        // Jade / forest-green stone walls
        vec3 wallCol = mix(vec3(0.03, 0.14, 0.08),
                           vec3(0.11, 0.40, 0.20),
                           fog);

        // Crease glow — brightest where the wall faces obliquely toward camera
        float rim    = pow(max(1.0 - abs(dot(n, -rd)), 0.0), 4.5);
        vec3  rimCol = vec3(0.28, 0.92, 0.52) * rim * fog * 0.65;

        col = wallCol * diff * fog + rimCol;
    }

    // Soft radial vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 1.40;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
