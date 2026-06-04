#version 150

in vec3 fragPos;
in vec4 fragColor;
in float vScreenY;

uniform vec3  uLightPos;
uniform vec3  uEyePos;
uniform float uTime;        // for animating the gradient
uniform float uObjHeight;   // world-space height of the object (for normalizing Y)
uniform float uLockGlow;    // 0..1, brightness pulse when figure clicks into solved pose
uniform int   uIsBall;      // 1 = render as bright emissive ball, 0 = normal figure

out vec4 outColor;

void main()
{
    // EARLY-OUT for the ball: bright emissive yellow, no shading, so it
    // reads as a "you-solved-it" indicator riding the figure rather than
    // a 3D object in the scene.
    if (uIsBall == 1) {
        outColor = vec4(fragColor.rgb * 2.0, 1.0);
        return;
    }
    // --- Compute face normal automatically from position derivatives ---
    // dFdx/dFdy give the rate of change of fragPos across adjacent pixels.
    // Their cross product is the face normal — no normal attribute needed.
    // This gives perfect flat shading for free on any geometry.
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    // --- Lighting: DIRECTIONAL light + CONSTANT view direction ---
    // These figures are drawn under orthographic (axonometric) projection, so
    // the view direction is the same for every fragment. Treating the light as
    // directional too makes the entire shade depend ONLY on the face normal —
    // never on world position. That is the key to a seamless solved join: the
    // two faces that align at the magic angle share a normal, so they receive
    // identical shading and read as one continuous surface (Inglis 2014 renders
    // its axonometric blocks the same way).
    vec3 L = normalize(uLightPos);
    vec3 V = normalize(uEyePos);
    vec3 R = reflect(-L, N);

    // --- Phong ---
    float ambient  = 0.20;
    float diffuse  = max(dot(N, L), 0.0) * 0.75;
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.6;

    // --- Escher gradient ---
    // Keyed to SCREEN-space height (not world Y). A vertical screen-space ramp
    // is continuous across an aligned join — both halves sit at the same screen
    // height, so they get the same brightness and no step appears at the seam —
    // while still giving the gentle top-lighter falloff + slow uTime shimmer.
    float gradientT = clamp(vScreenY, 0.0, 1.0);
    // Sine wave so average brightness stays constant regardless of view
    float brightness = 0.82 + 0.20 * sin(gradientT * 3.14159
                                        + uTime * 0.4);

    vec3 base     = fragColor.rgb;
    vec3 litColor = base * (ambient + diffuse) * brightness
                  + vec3(1.0) * specular;

    // --- Edge stylization (Inglis 2014 §Edge Classification) ---
    // Silhouette edges: surface normal nearly perpendicular to view
    // direction → dark outline around each block.
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);

    // Crease edges: at cube face-to-face boundaries the dFdx-derived
    // normal jumps discontinuously, so fwidth(N) spikes there.
    // Within a flat face fwidth(N) is ~0, and on a triangle diagonal
    // (within a quad) it's also ~0 because both triangles share the
    // same face normal — so we don't get spurious internal lines.
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));

    float edge       = max(silhouette, crease);
    vec3  edgeColor  = vec3(0.04, 0.04, 0.07);
    litColor         = mix(litColor, edgeColor, edge);

    // Lock-pulse glow: brief brightness bloom when the figure clicks into
    // its solved pose. Driven by CPU-side uLockGlow which is bumped to ~0.4
    // on solve and decays back to 0 over ~1s. Additive so it really pops.
    litColor += vec3(uLockGlow);
    litColor  = min(litColor, vec3(1.0));

    outColor = vec4(litColor, fragColor.a);
}