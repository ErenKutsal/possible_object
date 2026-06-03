#version 150

in vec2 fragUV;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uViewSize;
uniform float uTime;

out vec4 fragColor;

void main() {
    // Step 1 — reconstruct world-space ray direction for this pixel
    vec2 ndc = fragUV * 2.0 - 1.0;

    // For orthographic, all rays are parallel — the direction IS cam_forward.
    // But we want a spherical background that reacts to camera rotation,
    // so we construct a perspective-like direction from screen position.
    // This gives the illusion of looking around inside a sphere.
    vec3 ray_dir = normalize(uCamForward * 1.5
                           + ndc.x * uCamRight
                           + ndc.y * uCamUp);

    // Step 2 — convert ray direction to spherical coordinates
    // theta: horizontal angle (longitude), 0 to 2PI
    // phi:   vertical angle (latitude),    0 to PI
    float theta = atan(ray_dir.z, ray_dir.x);   // -PI to PI
    float phi   = acos(clamp(ray_dir.y, -1.0, 1.0)); // 0 to PI

    // Normalize to 0..1 for tiling
    float u = theta / (2.0 * 3.14159265);   // -0.5 to 0.5
    float v = phi   /       3.14159265;     //  0.0 to 1.0

    // Step 3 — Escher warp: two sine waves at different frequencies
    float warp_speed = uTime * 0.3;
    float wu = u + 0.15 * sin(v * 5.5 + warp_speed)
                 + 0.07 * sin(v * 11.0 - warp_speed * 1.3);
    float wv = v + 0.15 * sin(u * 5.5 + warp_speed * 0.7)
                 + 0.07 * sin(u * 11.0 + warp_speed * 0.9);

    // Step 4 — checkerboard on warped spherical coords
    // Scale controls how many tiles wrap around the sphere
    float scale = 6.0;
    float check = mod(floor(wu * scale) + floor(wv * scale), 2.0);

    // Step 5 — grid lines
    vec2  grid_uv  = fract(vec2(wu, wv) * scale);
    float line_w   = 0.04;
    float grid_line = step(1.0 - line_w, grid_uv.x)
                    + step(1.0 - line_w, grid_uv.y);
    grid_line = clamp(grid_line, 0.0, 1.0);

    // Step 6 — colors
    vec3 col_dark  = vec3(0.04, 0.03, 0.09);
    vec3 col_light = vec3(0.10, 0.08, 0.20);
    vec3 col_line  = vec3(0.35, 0.30, 0.60);

    vec3 tile_color = mix(col_dark, col_light, check);
    vec3 color      = mix(tile_color, col_line, grid_line * 0.8);

    // Step 7 — subtle vertical gradient so top feels like "sky"
    // and bottom feels like "depth" — reinforces the impossible space feel
    float sky_fade = smoothstep(0.0, 0.5, 1.0 - v);
    vec3  sky_tint = vec3(0.05, 0.03, 0.12);
    vec3  bot_tint = vec3(0.02, 0.02, 0.06);
    vec3  gradient = mix(bot_tint, sky_tint, sky_fade);
    color = mix(color, gradient, 0.3);  // 0.3 = tint strength, keep subtle

    fragColor = vec4(color, 1.0);
}