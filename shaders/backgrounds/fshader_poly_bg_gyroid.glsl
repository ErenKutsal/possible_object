#version 150
// Used by: polygon_bg.cpp | Object: Impossible Polygon (slot 0, bg index 2) | Effect: Gyroid SDF background

// ─────────────────────────────────────────────────────────────────────────────
// Gyroid SDF  (n=5, pentagon slot)
//
// Ray-marches a triply-periodic gyroid surface
//   |sin(x)cos(y) + sin(y)cos(z) + sin(z)cos(x)| = ε
// Camera-aware: the polygon's orbit camera shifts the view angle.
// Dark navy + teal + rim-light cyan palette.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTime;
uniform float uAspect;

// ── Gyroid thin-shell SDF ────────────────────────────────────────────────────
float mapGyroid(vec3 p)
{
    // Scale + slow drift so it swims gently
    p *= 1.5;
    p.z += uTime * 0.25;
    p.x += uTime * 0.08;

    float g = sin(p.x) * cos(p.y)
            + sin(p.y) * cos(p.z)
            + sin(p.z) * cos(p.x);

    // Thin shell around the level set g = 0
    return abs(g) - 0.08;
}

vec3 gyroidNormal(vec3 p)
{
    const float e = 0.004;
    return normalize(vec3(
        mapGyroid(p + vec3(e, 0, 0)) - mapGyroid(p - vec3(e, 0, 0)),
        mapGyroid(p + vec3(0, e, 0)) - mapGyroid(p - vec3(0, e, 0)),
        mapGyroid(p + vec3(0, 0, e)) - mapGyroid(p - vec3(0, 0, e))
    ));
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Camera-aware ray — polygon orbital camera drives parallax
    vec3 ro = uEyePos * 1.2 + vec3(uTime * 0.06, 0.0, uTime * 0.06);
    vec3 rd = normalize(uCamForward + uCamRight * uv.x * 0.65 + uCamUp * uv.y * 0.65);

    // ── Ray march ──
    float t   = 0.0;
    bool  hit = false;
    for (int i = 0; i < 80; i++) {
        float d = mapGyroid(ro + rd * t);
        if (d < 0.003) { hit = true; break; }
        if (t > 18.0)  break;
        t += d * 0.65;
    }

    // ── Background colour when no hit ──
    vec3 col = vec3(0.015, 0.028, 0.055);

    if (hit) {
        vec3 p = ro + rd * t;
        vec3 n = gyroidNormal(p);

        float diff = max(dot(n, normalize(vec3(1.0, 1.0, 0.5))), 0.0);
        float rim  = pow(1.0 - abs(dot(n, rd)), 3.0);
        float fog  = exp(-t * 0.22);

        // Dark teal diffuse + cyan rim
        vec3 baseCol = mix(vec3(0.015, 0.028, 0.055),
                           mix(vec3(0.04, 0.20, 0.28), vec3(0.12, 0.60, 0.70), diff),
                           fog);
        vec3 rimCol  = vec3(0.35, 0.90, 1.00) * rim * fog * 0.8;

        col = baseCol + rimCol;
    }

    // Radial vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 1.1;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
