#version 150
// ─────────────────────────────────────────────────────────────────────────────
// Julia Set  (n=4, square slot)
//
// Animated Julia-set fractal.  The parameter c traces a circle so the fractal
// continuously morphs — a subtle nod to the square's four-fold symmetry.
// Dark indigo → electric violet → cyan palette.
// ─────────────────────────────────────────────────────────────────────────────

in  vec2 fragUV;
out vec4 FragColor;

uniform float uTime;
uniform float uAspect;

// ── Smooth iteration count ──────────────────────────────────────────────────
float julia(vec2 z, vec2 c)
{
    for (int i = 0; i < 96; i++) {
        if (dot(z, z) > 4.0) return float(i) - log2(log2(dot(z, z))) + 4.0;
        z = vec2(z.x * z.x - z.y * z.y + c.x,
                 2.0 * z.x * z.y       + c.y);
    }
    return -1.0;  // in-set
}

// ── Colour ramp (indigo → violet → electric cyan) ───────────────────────────
vec3 fractalColor(float t)
{
    // Three-stop cosine palette
    vec3 a = vec3(0.5);
    vec3 b = vec3(0.5);
    vec3 c = vec3(1.0, 0.7, 0.4);
    vec3 d = vec3(0.00, 0.25, 0.67);
    return a + b * cos(6.28318 * (c * t + d));
}

void main()
{
    vec2 uv = (fragUV - 0.5) * 3.4;
    uv.x *= uAspect;

    // c traces a petal around the boundary of the Mandelbrot set
    float ct  = uTime * 0.10;
    vec2  c   = vec2(0.7885 * cos(ct), 0.7885 * sin(ct));

    // Slight drift to give the illusion the camera is slowly zooming in
    float zoom = 1.0 + 0.06 * sin(uTime * 0.07);
    uv *= zoom;

    float v = julia(uv, c);

    vec3 col;
    if (v < 0.0) {
        // In-set — very dark navy so the glowing boundary pops
        col = vec3(0.01, 0.01, 0.06);
    } else {
        float t = v / 96.0;
        col = fractalColor(t * 1.8 + uTime * 0.04);
        // Darken the far field so the bright boundary stands out
        col *= mix(0.15, 1.0, pow(t, 0.45));
    }

    // Soft radial vignette
    float vig = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 0.9;
    col *= clamp(vig, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
