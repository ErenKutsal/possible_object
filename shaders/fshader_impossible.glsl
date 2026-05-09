#version 150

in vec3 fragPos;
in vec4 fragColor;

uniform vec3  uLightPos;
uniform vec3  uEyePos;
uniform float uTime;        // for animating the gradient
uniform float uObjHeight;   // world-space height of the object (for normalizing Y)

out vec4 outColor;

void main()
{
    // --- Compute face normal automatically from position derivatives ---
    // dFdx/dFdy give the rate of change of fragPos across adjacent pixels.
    // Their cross product is the face normal — no normal attribute needed.
    // This gives perfect flat shading for free on any geometry.
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    vec3 L = normalize(uLightPos - fragPos);
    vec3 V = normalize(uEyePos   - fragPos);
    vec3 R = reflect(-L, N);

    // --- Phong ---
    float ambient  = 0.20;
    float diffuse  = max(dot(N, L), 0.0) * 0.75;
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.6;

    // --- Escher gradient ---
    // Normalize world Y into 0..1 range across the object's height.
    // Higher faces appear lighter — creates impossible depth no matter
    // what the object is or how it is oriented.
    float gradientT = clamp(fragPos.y / uObjHeight + 0.5, 0.0, 1.0);
    // Sine wave so average brightness stays constant regardless of view
    float brightness = 0.82 + 0.20 * sin(gradientT * 3.14159
                                        + uTime * 0.4);

    vec3 base     = fragColor.rgb;
    vec3 litColor = base * (ambient + diffuse) * brightness
                  + vec3(1.0) * specular;

    outColor = vec4(litColor, fragColor.a);
}