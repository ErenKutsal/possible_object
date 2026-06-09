#version 150

in float fArcLength;

uniform float uPulsePos1;
uniform float uPulseIntensity1;
uniform float uPulsePos2;
uniform float uPulseIntensity2;
uniform float uTotalLength;
uniform vec3 uColor;

out vec4 outColor;

float computeGlow(float dist)
{
    // Increased core and wider halo for significantly higher visibility
    float core = smoothstep(1.0, 0.0, dist);
    float halo = exp(-dist * 2.5) * 0.8;
    return core + halo;
}

void main()
{
    // Find shortest distance along the closed loop (with wrap-around)
    float dist1 = abs(fArcLength - uPulsePos1);
    dist1 = min(dist1, uTotalLength - dist1);

    float dist2 = abs(fArcLength - uPulsePos2);
    dist2 = min(dist2, uTotalLength - dist2);

    float pulseWidth = 4.5; // Wider pulse width
    float normDist1 = dist1 / pulseWidth;
    float normDist2 = dist2 / pulseWidth;

    float glow1 = computeGlow(normDist1) * uPulseIntensity1;
    float glow2 = computeGlow(normDist2) * uPulseIntensity2;

    float totalGlow = max(glow1, glow2);

    // Elevated baseline visibility (0.32 vs 0.12)
    float baseline = 0.32;
    vec3 baseColor = uColor * 0.8; // Brighter baseline color
    vec3 pulseColor = vec3(0.0, 0.88, 1.0); // Intense neon cyan

    vec3 finalColor = mix(baseColor, pulseColor, totalGlow);
    float finalAlpha = mix(baseline, 1.0, totalGlow);

    outColor = vec4(finalColor, finalAlpha);
}
