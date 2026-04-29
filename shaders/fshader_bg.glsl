#version 150

in vec2 fragUV;

uniform vec3  uEyePos;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uViewSize;
uniform float uTime;
uniform float uFloorY;

out vec4 fragColor;

void main() {
    // Step 1 — reconstruct world-space ray for this pixel
    // NDC coordinates: -1 to +1
    vec2 ndc = fragUV * 2.0 - 1.0;

    // In orthographic projection, every ray is parallel to cam_forward.
    // The ray origin shifts laterally based on screen position.
    vec3 ray_origin = uEyePos
                    + ndc.x * uViewSize * uCamRight
                    + ndc.y * uViewSize * uCamUp;
    vec3 ray_dir    = uCamForward;

    // Step 2 — intersect ray with horizontal floor plane (y = uFloorY)
    float denom = ray_dir.y;
    if (abs(denom) < 0.001) {
        // Ray is parallel to floor — just show deep background color
        fragColor = vec4(0.04, 0.04, 0.10, 1.0);
        return;
    }

    float t   = (uFloorY - ray_origin.y) / denom;
    vec3  hit = ray_origin + t * ray_dir;

    // Step 3 — Escher warp: distort grid coordinates with sine waves
    // Two waves at different frequencies create an Escher-like ripple
    float warp_speed = uTime * 0.4;
    float wx = hit.x + 0.25 * sin(hit.z * 1.8 + warp_speed)
                     + 0.12 * sin(hit.z * 3.7 - warp_speed * 1.3);
    float wz = hit.z + 0.25 * sin(hit.x * 1.8 + warp_speed * 0.7)
                     + 0.12 * sin(hit.x * 3.7 + warp_speed * 0.9);

    // Step 4 — checkerboard on warped coordinates
    float check = mod(floor(wx * 2.0) + floor(wz * 2.0), 2.0);

    // Step 5 — grid lines (thin bright lines at tile borders)
    vec2  grid_uv   = fract(vec2(wx, wz) * 2.0);
    float line_w    = 0.04;
    float grid_line = step(1.0 - line_w, grid_uv.x)
                    + step(1.0 - line_w, grid_uv.y);
    grid_line = clamp(grid_line, 0.0, 1.0);

    // Step 6 — colors
    // Dark squares: very dark blue-purple
    // Light squares: slightly lighter, still dark overall so object pops
    vec3 col_dark  = vec3(0.04, 0.03, 0.09);
    vec3 col_light = vec3(0.10, 0.08, 0.20);
    vec3 col_line  = vec3(0.35, 0.30, 0.60);  // purple-ish grid lines

    vec3 tile_color = mix(col_dark, col_light, check);
    vec3 color      = mix(tile_color, col_line, grid_line * 0.8);

    // Step 7 — distance fade to background color so horizon doesn't pop
    float dist      = length(hit - uEyePos);
    float fade      = exp(-dist * 0.5);
    vec3  bg_color  = vec3(0.03, 0.02, 0.07);  // deep space background
    color           = mix(bg_color, color, fade * fade);

    // Step 8 — only draw floor if ray hit it in front of camera (t > 0)
    if (t < 0.0)
        color = bg_color;

    fragColor = vec4(color, 1.0);
}