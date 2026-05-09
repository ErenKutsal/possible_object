#version 150

in  vec2 fragUV;
out vec4 fragColor;

uniform float uTime;
uniform float uAspect;
uniform vec2  uC;       // the Julia constant — animated from C++
uniform float uZoom;

// --- Cosine color palette (Inigo Quilez technique) ---
// Produces smooth, controllable color gradients from a single float.
// Formula: color = a + b * cos(2π * (c*t + d))
// By tuning a,b,c,d you get completely different looks.
vec3 palette(float t)
{
    vec3 a = vec3(0.5,  0.5,  0.5);
    vec3 b = vec3(0.5,  0.5,  0.5);
    vec3 c = vec3(1.0,  1.0,  1.0);
    vec3 d = vec3(0.30, 0.20, 0.50); // phase offsets — controls hue shift
    return a + b * cos(6.28318 * (c * t + d));
}

void main()
{
    // Step 1 — map screen UV to complex plane, centered at origin
    vec2 uv  = fragUV * 2.0 - 1.0;        // -1 to +1
    uv.x    *= uAspect;                     // correct for non-square screens
    vec2 z   = uv * uZoom;                  // apply zoom

    // Step 2 — Julia iteration
    // z_{n+1} = z_n^2 + c, using complex multiplication:
    //   Re(z^2) = x^2 - y^2
    //   Im(z^2) = 2xy
    const int MAX_ITER = 128;
    int i = 0;
    for (; i < MAX_ITER; i++)
    {
        // Escape condition: |z|^2 > 4  (cheaper than sqrt)
        if (dot(z, z) > 4.0) break;

        z = vec2(z.x * z.x - z.y * z.y,
                 2.0 * z.x * z.y) + uC;
    }

    // Step 3 — smooth coloring
    // Raw iteration count produces harsh color bands at set boundaries.
    // The log2(log2()) correction smooths these into continuous gradients.
    // This works because |z| grows exponentially after escape.
    if (i == MAX_ITER)
    {
        // Inside the set — draw dark, slightly colored
        // Tiny inner glow based on final |z| makes it feel alive
        float inner = dot(z, z) / 4.0;
        fragColor   = vec4(0.02, 0.01, 0.05 + inner * 0.03, 1.0);
        return;
    }

    float smooth_t = float(i) - log2(log2(dot(z, z)));

    // Step 4 — map to color
    // Divide by MAX_ITER to get 0..1, add time for slow color cycling
    float color_t  = smooth_t / float(MAX_ITER);
    color_t        = fract(color_t * 3.0 + uTime * 0.05); // 3 = color repeat

    vec3 color = palette(color_t);

    // Step 5 — darken toward the center of the set (near-escape points)
    // This gives the set boundary a crisp glowing edge
    float edge_glow = 1.0 - float(i) / float(MAX_ITER);
    color *= 0.8 + 0.5 * edge_glow;

    // Step 6 — vignette: darken screen corners so object stays focal point
    float vignette = 1.0 - dot(fragUV - 0.5, fragUV - 0.5) * 1.2;
    vignette       = clamp(vignette, 0.0, 1.0);
    color         *= vignette;

    fragColor = vec4(color, 1.0);
}