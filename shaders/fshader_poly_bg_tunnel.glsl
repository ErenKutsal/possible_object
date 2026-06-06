#version 150
// ─────────────────────────────────────────────────────────────────────────────
// Neon Polygon Tunnel  (n=6+, hexagon slot and beyond)
//
// The camera flies through an infinite tunnel whose cross-section is a regular
// N-gon — the tunnel sides match the polygon count, so a hexagon shows a
// hexagonal tunnel.  Camera-aware: the orbit camera tilts the vanishing point.
// Hot magenta / cyan neon palette — high-energy contrast to the sage polygon.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTime;
uniform float uAspect;
uniform int   uNSides;   // polygon side count (drives tunnel cross-section)

const float PI = 3.14159265;

// ── Regular-N-gon SDF in 2D ─────────────────────────────────────────────────
float sdNgon(vec2 p, float r, int n)
{
    float a = atan(p.y, p.x) + PI / float(n);
    float b = PI * 2.0 / float(n);
    vec2  q = vec2(cos(floor(0.5 + a / b) * b - PI / float(n)),
                   sin(floor(0.5 + a / b) * b - PI / float(n))) * r;
    return length(p - q * dot(p, q) / dot(q, q)) * sign(p.x * q.y - p.y * q.x);
}

// ── Tunnel SDF: distance to the inner wall of the N-gon cross-section tube ──
float mapTunnel(vec3 p, float twist)
{
    // Slow twist along Z
    float angle = p.z * 0.10 + twist;
    float c = cos(angle), s = sin(angle);
    vec2  xy = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

    // Modular Z: tile every 6 units (creates ring decorations at seams)
    float zmod = mod(p.z, 6.0);

    // Cross-section radius pulses slightly at seam positions
    float rPulse = 1.55 + 0.05 * sin(zmod * PI / 3.0);

    return -sdNgon(xy, rPulse, max(uNSides, 3));  // negative = inside
}

// ── Neon stripe pattern on the walls ────────────────────────────────────────
vec3 wallColor(vec3 p, vec3 n, float t)
{
    float zmod  = mod(p.z, 6.0);
    float stripe = 0.5 + 0.5 * sin(zmod * PI / 0.3 + uTime * 2.0);

    vec3 hot  = vec3(1.0, 0.10, 0.60);   // magenta
    vec3 cool = vec3(0.05, 0.85, 1.00);  // cyan
    vec3 base = mix(hot, cool, fract(t * 0.12 + uTime * 0.05));

    // Emit on stripes for bloom
    float emit = pow(stripe, 6.0) * 1.5;
    return base * (0.3 + emit);
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Ray origin: camera drifts forward through the tunnel
    vec3 ro = uEyePos * 0.5 + vec3(0.0, 0.0, uTime * 1.2);
    // Slight camera tilt from the polygon's orbital angle
    vec3 rd = normalize(uCamForward + uCamRight * uv.x * 0.70 + uCamUp * uv.y * 0.70);

    float twist = uTime * 0.12;

    // ── Ray march ──
    float t   = 0.0;
    bool  hit = false;
    for (int i = 0; i < 100; i++) {
        float d = mapTunnel(ro + rd * t, twist);
        if (d < 0.004) { hit = true; break; }
        if (t > 40.0)  break;
        t += max(abs(d) * 0.8, 0.01);
    }

    vec3 col = vec3(0.0);
    if (hit) {
        vec3 p = ro + rd * t;

        // Finite-difference normal
        const float e = 0.004;
        vec3 n = normalize(vec3(
            mapTunnel(p + vec3(e, 0, 0), twist) - mapTunnel(p - vec3(e, 0, 0), twist),
            mapTunnel(p + vec3(0, e, 0), twist) - mapTunnel(p - vec3(0, e, 0), twist),
            mapTunnel(p + vec3(0, 0, e), twist) - mapTunnel(p - vec3(0, 0, e), twist)
        ));

        float fog  = exp(-t * 0.045);
        col = wallColor(p, n, t) * fog;

        // Specular glint for the neon sheen
        vec3 ref   = reflect(rd, n);
        float spec = pow(max(dot(ref, -rd), 0.0), 24.0);
        col += vec3(1.0) * spec * fog * 0.6;
    }

    // Vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 1.1;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
