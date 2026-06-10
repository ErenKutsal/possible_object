#version 150
// Used by: penrose_blocks_proc.cpp | Object: Blocked Penrose | Effect: Dark background with a soft blueish center glow

in vec2 vNdc;
out vec4 outColor;

void main()
{
    // Dark base color (almost black/deep navy)
    vec3 baseColor = vec3(0.04, 0.04, 0.07);

    // Soft radial blue glow centered in the middle of the screen
    float dist = length(vNdc);
    float glow = exp(-dist * dist * 1.5);
    vec3 glowColor = vec3(0.12, 0.22, 0.40); // soft blue

    vec3 col = mix(baseColor, glowColor, glow * 0.75);

    outColor = vec4(col, 1.0);
}
