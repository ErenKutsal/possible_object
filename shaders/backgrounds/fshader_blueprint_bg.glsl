#version 150

// Blueprint Mathematical Grid background — fragment shader.
// Draws crisp, analytical 1-pixel wide grids, polar circles, and dotted guides.
// Perfectly complements the "mathematically intriguing" Penrose blocks.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;
uniform float uTime;
uniform float uAmount;
uniform float uAspect;

void main()
{
    // Deep premium blueprint blue void
    vec3 baseCol = vec3(0.043, 0.059, 0.10);
    
    // Radial gradient glow centered on the screen
    float dCenter = length(vNdc);
    float glow = exp(-dCenter * dCenter * 1.2);
    vec3 glowCol = vec3(0.08, 0.12, 0.22);
    
    vec3 col = mix(baseCol, glowCol, glow);
    
    if (uAmount > 0.001)
    {
        // Correct for screen aspect ratio
        vec2 uv = vNdc * vec2(uAspect, 1.0);
        
        // 1. Grid of fine coordinate lines
        vec2 st = uv * 3.5;
        vec2 grid_coord = abs(fract(st - 0.5) - 0.5) / fwidth(st);
        float grid_line = 1.0 - min(min(grid_coord.x, grid_coord.y), 1.0);
        
        // 2. Concentric circle polar grids
        float r = length(uv) * 2.2;
        float circle_coord = abs(fract(r - 0.5) - 0.5) / fwidth(r);
        float circle_line = 1.0 - min(circle_coord, 1.0);
        
        // Dotted circles based on angle
        float angle = atan(uv.y, uv.x);
        float dotted_circle = circle_line * step(0.0, sin(angle * 36.0));
        
        // 3. Diagonal auxiliary guide lines (30/60 degree steps)
        float PI = 3.14159265;
        float ray_factor = angle * (12.0 / (2.0 * PI));
        float ray_coord = abs(fract(ray_factor - 0.5) - 0.5) / fwidth(ray_factor);
        float ray_line = 1.0 - min(ray_coord, 1.0);
        
        // Dotted ray lines based on radius
        float dotted_ray = ray_line * step(0.0, sin(length(uv) * 10.0));
        
        // Palette of technical drafting lines
        vec3 gridColor = vec3(0.12, 0.18, 0.30); // blueprint blue-grey
        vec3 guideColor = vec3(0.08, 0.22, 0.26); // blueprint cyan-teal
        
        // Combine layers
        vec3 blueprintLayer = col;
        blueprintLayer = mix(blueprintLayer, gridColor, grid_line * 0.25);
        blueprintLayer = mix(blueprintLayer, guideColor, dotted_circle * 0.20);
        blueprintLayer = mix(blueprintLayer, guideColor * 0.8, dotted_ray * 0.15);
        
        // Subtle blueprint sweep highlight
        float sweep = sin(uv.x * 2.0 - uTime * 0.5) * 0.5 + 0.5;
        blueprintLayer += gridColor * grid_line * sweep * 0.08;
        
        col = mix(col, blueprintLayer, uAmount);
    }
    
    outColor = vec4(col, 1.0);
}
