#version 150

in vec2 vNdc;

out vec4 outColor;

void main()
{
    // A soft radial gradient for the cyclo studio backdrop with increased contrast
    float dist = length(vNdc);
    vec3 centerColor = vec3(0.90, 0.90, 0.92); 
    vec3 edgeColor   = vec3(0.70, 0.72, 0.76); 
    vec3 col = mix(centerColor, edgeColor, smoothstep(0.0, 1.414, dist));

    // Subtle mathematical grid overlay (faint lines)
    vec2 gridUV = vNdc * 14.0; 
    vec2 grid = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
    float line = min(grid.x, grid.y);
    float gridPattern = 1.0 - min(line, 1.0);

    // Fade the grid out towards the edges of the screen
    float gridFade = smoothstep(1.3, 0.2, dist);
    vec3 gridColor = vec3(0.50, 0.54, 0.58); // darker grey for higher visibility
    col = mix(col, gridColor, gridPattern * 0.35 * gridFade);

    outColor = vec4(col, 1.0);
}
