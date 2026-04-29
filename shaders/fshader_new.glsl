#version 150

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragLocalNormal;

uniform vec3  uLightPos;
uniform vec3  uEyePos;
uniform vec3  uBaseColor;
uniform float uBarT;       
uniform int   uNumSegments;

// --- New Ball Light Variables ---
uniform vec3  uBallPos;
uniform int   uIsBall;

out vec4 fragColor;

void main() {
    // 1. EMISSION: If this is the ball, make it glow brightly and skip shadows
    if (uIsBall == 1) {
        // Multiply > 1.0 so your bloom pass triggers
        fragColor = vec4(uBaseColor * 2.0, 1.0); 
        return;
    }

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(uLightPos - fragPos);
    vec3 V = normalize(uEyePos   - fragPos);
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
    float attenuation = 1.0 / (1.0 + 15.0 * dist * dist); 
    
    float diff_ball = max(dot(N, L_ball), 0.0);
    float spec_ball = pow(max(dot(R_ball, V), 0.0), 64.0);

    vec3 neonColor = vec3(1.0, 0.85, 0.2); // Golden yellow glow

    // --- Combine Everything ---
    // Apply the Escher illusion to the base lighting...
    vec3 baseLighting = uBaseColor * (ambient + diffuse) * brightness;
    
    // ...then add the global shine and the dynamic ball light on top!
    vec3 ballLighting = (uBaseColor * diff_ball * 2.0 + neonColor * spec_ball * 1.5) * attenuation * neonColor;
    
    vec3 litColor = baseLighting + (vec3(1.0) * specular) + ballLighting;

    fragColor = vec4(litColor, 1.0);
}