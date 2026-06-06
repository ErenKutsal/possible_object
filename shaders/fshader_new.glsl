#version 150

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragLocalNormal;

uniform vec3  uLightPos;
uniform vec3  uEyePos;
uniform vec3  uBaseColor;
uniform float uBarT;       
uniform int   uNumSegments;
uniform float uLockGlow;   // 0 = unlocked, >0 = locked (pushes to HDR for bloom)

// --- New Ball Light Variables ---
uniform vec3  uBallPos;
uniform int   uIsBall;
uniform vec3  uBallColor;

out vec4 fragColor;

void main() {
    // 1. EMISSION: If this is the ball, make it glow brightly and skip shadows
    if (uIsBall == 1) {
        // Multiply > 1.0 so your bloom pass triggers
        fragColor = vec4(uBaseColor * 2.0, 1.0); 
        return;
    }

    vec3 N = normalize(fragNormal);
    // Global light = DIRECTIONAL, view direction = CONSTANT (correct under the
    // orthographic projection this figure uses). The global Phong term then
    // depends only on the face normal — never on world position — so the two
    // halves of the split bar 0 (which share a normal where they align at the
    // magic angle, just sitting at different depths) receive identical shading
    // and read as one continuous, seamless bar. The dynamic ball light below
    // stays a real positional point light — it's a moving indicator, not part
    // of the static illusion surface.
    vec3 L = normalize(uLightPos);
    vec3 V = normalize(uEyePos);
    vec3 R = reflect(-L, N);

    // --- Global Light (Your original Phong setup) ---
    float ambient   = 0.20;
    float diffuse   = max(dot(N, L), 0.0) * 0.7;
    float specular  = pow(max(dot(R, V), 0.0), 64.0) * 0.7;

    // --- Escher gradient (The Optical Illusion Magic) ---
    float faceDepth = dot(normalize(fragLocalNormal), vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    float gradientT = fract(uBarT + (1.0 - faceDepth) / float(uNumSegments));
    float brightness = 0.85 + 0.18 * sin(gradientT * 2.0 * 3.14159265);

    // --- Dynamic Ball Light ---
    vec3 L_ball = uBallPos - fragPos;
    float dist = length(L_ball);
    L_ball = normalize(L_ball);
    vec3 R_ball = reflect(-L_ball, N);

    // Attenuation makes the light fade naturally over distance
    float attenuation = 1.0 / (1.0 + 10.0 * dist * dist);

    // Half-Lambert wrapping: maps dot(N, L_ball) from [-1,1] to [0,1].
    // This ensures the ball casts a warm glow even on back-facing surfaces
    // (inner face of the loop), where a hard clamp to 0 would kill the light.
    float diff_ball = dot(N, L_ball) * 0.5 + 0.5;
    diff_ball = diff_ball * diff_ball;  // square for tighter falloff
    float spec_ball = pow(max(dot(R_ball, V), 0.0), 32.0);

    vec3 neonColor = uBallColor; // Color-matched neon glow

    // --- Combine Everything ---
    // Apply the Escher illusion to the base lighting...
    vec3 baseLighting = uBaseColor * (ambient + diffuse) * brightness;
    
    // ...then add the global shine and the dynamic ball light on top!
    vec3 ballLighting = (uBaseColor * diff_ball * 1.6 + neonColor * spec_ball * 2.0) * attenuation * neonColor;
    
    vec3 litColor = baseLighting + (vec3(1.0) * specular) + ballLighting;

    // HDR boost when locked — pushes bright surfaces above 1.0 so the bloom
    // bright-pass catches them and adds a satisfying "solved" glow.
    litColor *= (1.0 + uLockGlow * 3.0);

    fragColor = vec4(litColor, 1.0);
}