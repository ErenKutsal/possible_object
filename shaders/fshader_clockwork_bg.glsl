#version 150

// Astral Clockwork background — fades in once the Penrose Staircase
// is solved. Uses the fullscreen-quad vertex shader (vshader_halo.glsl).
//
// Layered effect:
//   1. Deep space void — deep indigo/black gradient.
//   2. 3D Ray-marched SDF Gears — an intricate clockwork mechanism that
//      rotates continuously to symbolize the infinite staircase loop.
//   3. Deep volumetric atmospheric fog and glow.
//   4. uAmount blends everything in from the pale-slate clear colour.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;
uniform float uTime;
uniform float uAmount;

const int   MAX_STEPS = 96;
const float MAX_DIST  = 30.0;
const float SURF_DIST = 0.005;

// ── Rotation ────────────────────────────────────────────────────────────────
mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c,-s,s,c); }

// ── SDF Primitives ──────────────────────────────────────────────────────────
float sdCylinder(vec3 p, vec2 h) {
    vec2 d = abs(vec2(length(p.xy), p.z)) - h;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

// A gear SDF: a cylinder base minus inner holes, plus outer teeth
float sdGear(vec3 p, float radius, float thickness, float teethCount, float teethDepth, float teethWidth, float timeOffset, float speed) {
    // Rotate gear
    p.xy = rot(uTime * speed + timeOffset) * p.xy;
    
    // Base cylinder
    float d = sdCylinder(p, vec2(radius, thickness));
    
    // Center hole
    float hole = sdCylinder(p, vec2(radius * 0.35, thickness * 1.5));
    d = max(d, -hole);
    
    // Spokes / cutouts
    float a = atan(p.y, p.x);
    float spokes = length(p.xy) - radius * 0.45;
    float cutAngle = mod(a, 3.14159265 * 2.0 / 5.0) - 3.14159265 / 5.0;
    float cutouts = max(abs(cutAngle * length(p.xy)) - radius * 0.3, abs(p.z) - thickness * 1.5);
    d = max(d, -(cutouts + min(spokes, 0.0))); // Rough cutouts

    // Teeth
    float teethAngle = mod(a, 3.14159265 * 2.0 / teethCount) - 3.14159265 / teethCount;
    // Project p onto local tooth space
    vec3 tp = vec3(length(p.xy) - radius, teethAngle * radius, p.z);
    float teeth = sdCylinder(tp.xzy, vec2(teethWidth, teethDepth));
    
    // The base cylinder is 'radius' wide, but teeth stick out to 'radius + teethDepth'
    // Actually simpler: just combine base with teeth cylinders arranged radially
    // Wait, the sdCylinder for tp is wrong because it's a box.
    // Box SDF:
    vec3 q = abs(tp) - vec3(teethDepth, teethWidth, thickness);
    float tBox = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
    
    return min(d, tBox);
}

// ── Scene SDF ───────────────────────────────────────────────────────────────
float sceneSDF(vec3 p) {
    float d = MAX_DIST;
    
    // Gear 1: Large background gear
    vec3 p1 = p - vec3(0.0, 0.0, 5.0);
    p1.xz = rot(0.2) * p1.xz;
    float g1 = sdGear(p1, 3.5, 0.2, 32.0, 0.4, 0.2, 0.0, 0.15);
    d = min(d, g1);
    
    // Gear 2: Medium interlocking gear, front left
    vec3 p2 = p - vec3(-3.2, -1.5, 3.0);
    p2.yz = rot(0.5) * p2.yz;
    p2.xz = rot(-0.3) * p2.xz;
    float g2 = sdGear(p2, 2.0, 0.3, 16.0, 0.4, 0.2, 0.4, -0.3); // opposite rotation
    d = min(d, g2);

    // Gear 3: Small fast gear, front right
    vec3 p3 = p - vec3(2.5, 2.0, 2.5);
    p3.xz = rot(0.6) * p3.xz;
    p3.yz = rot(-0.4) * p3.yz;
    float g3 = sdGear(p3, 1.2, 0.15, 12.0, 0.3, 0.15, 1.2, 0.6);
    d = min(d, g3);

    // Gear 4: Very large slow ring far back
    vec3 p4 = p - vec3(1.0, 1.0, 8.0);
    float g4 = sdGear(p4, 6.0, 0.4, 48.0, 0.5, 0.25, 0.0, -0.05);
    d = min(d, g4);
    
    return d;
}

// ── Normal Calculation ──────────────────────────────────────────────────────
vec3 calcNormal(vec3 p) {
    const float e = 0.005;
    return normalize(vec3(
        sceneSDF(p + vec3(e, 0.0, 0.0)) - sceneSDF(p - vec3(e, 0.0, 0.0)),
        sceneSDF(p + vec3(0.0, e, 0.0)) - sceneSDF(p - vec3(0.0, e, 0.0)),
        sceneSDF(p + vec3(0.0, 0.0, e)) - sceneSDF(p - vec3(0.0, 0.0, e))
    ));
}

void main()
{
    vec3 finalColor = uBaseColor;

    if (uAmount > 0.001)
    {
        // Aspect ratio correction (vNdc is -1 to 1)
        // Assume 1.0 aspect for now since we don't pass it to the background shader,
        // but vNdc is typically just screenspace. The clockwork is abstract enough
        // that slight stretching doesn't hurt, but let's just use it directly.
        vec2 uv = vNdc;

        // Camera setup
        vec3 ro = vec3(0.0, 0.0, -4.0); // Eye
        // Subtle camera pan
        ro.x += sin(uTime * 0.1) * 0.5;
        ro.y += cos(uTime * 0.15) * 0.5;
        
        vec3 rd = normalize(vec3(uv, 1.5)); // Ray dir (FOV approx 45 deg)
        // Look at center
        vec3 target = vec3(0.0, 0.0, 0.0);
        vec3 fwd = normalize(target - ro);
        vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
        vec3 up = cross(fwd, right);
        rd = normalize(right * uv.x + up * uv.y + fwd * 1.5);

        // Ray march
        float t = 0.0;
        bool hit = false;
        for (int i = 0; i < MAX_STEPS; i++) {
            vec3 p = ro + rd * t;
            float d = sceneSDF(p);
            if (d < SURF_DIST) { hit = true; break; }
            if (t > MAX_DIST) break;
            t += d * 0.8;
        }

        vec3 col = vec3(0.0);

        // Deep space void gradient
        vec3 spaceVoid = mix(vec3(0.00, 0.01, 0.04), vec3(0.05, 0.02, 0.10), length(uv));

        if (hit) {
            vec3 p = ro + rd * t;
            vec3 n = calcNormal(p);

            // Lighting setup
            vec3 lightPos1 = vec3(2.0, 4.0, -2.0);
            vec3 l1 = normalize(lightPos1 - p);
            
            vec3 lightPos2 = vec3(-4.0, -2.0, 1.0);
            vec3 l2 = normalize(lightPos2 - p);

            // Brass / Gold material
            vec3 albedo = vec3(0.18, 0.10, 0.05); // dark bronze base
            
            // Key light (warm golden)
            float diff1 = max(dot(n, l1), 0.0);
            float spec1 = pow(max(dot(reflect(-l1, n), -rd), 0.0), 32.0);
            vec3 lit1 = vec3(1.0, 0.7, 0.3) * diff1 + vec3(1.0, 0.9, 0.6) * spec1 * 0.8;
            
            // Fill light (cool indigo)
            float diff2 = max(dot(n, l2), 0.0);
            float spec2 = pow(max(dot(reflect(-l2, n), -rd), 0.0), 16.0);
            vec3 lit2 = vec3(0.1, 0.2, 0.4) * diff2 + vec3(0.2, 0.4, 0.8) * spec2 * 0.3;

            // Ambient / rim light
            float rim = pow(clamp(1.0 - dot(n, -rd), 0.0, 1.0), 3.0);
            vec3 amb = vec3(0.6, 0.4, 0.1) * rim * 0.5;

            col = albedo * (vec3(0.05) + lit1 + lit2) + lit1*0.2 + lit2*0.1 + amb;

            // Fog (deep space atmosphere)
            float fog = exp(-t * 0.12);
            col = mix(spaceVoid, col, fog);
        } else {
            col = spaceVoid;
        }

        // Vignette
        float vig = 1.0 - dot(vNdc, vNdc) * 0.35;
        col *= clamp(vig, 0.0, 1.0);

        finalColor = mix(uBaseColor, col, uAmount);
    }

    outColor = vec4(finalColor, 1.0);
}
