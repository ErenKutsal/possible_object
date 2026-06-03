#version 330 core

in vec2 fragUV;
out vec4 FragColor;

uniform vec3 uEyePos;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform vec3 uCamForward;
uniform float uTime;
uniform float uAspect;

// 2D Rotation matrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Distance to the Gyroid surface
float map(vec3 p) {
    // Slowly rotate the entire structure to make it feel disorienting
    p.xz *= rot(uTime * 0.1);
    p.xy *= rot(uTime * 0.05);

    // Scale down the coordinates to make the arches larger
    p *= 1.5; 
    
    // The Gyroid formula: dot(sin(p), cos(p.zxy))
    // We take the absolute value and subtract a thickness to make hollow walls/arches
    float thickness = 0.15;
    float d = abs(dot(sin(p), cos(p.zxy))) - thickness;
    
    // Divide by the scale to fix the raymarching distance
    return d / 1.5; 
}

// Calculate the normal vector at the surface for lighting
vec3 getNormal(vec3 p) {
    vec2 e = vec2(0.01, 0.0);
    return normalize(vec3(
        map(p + e.xyy) - map(p - e.xyy),
        map(p + e.yxy) - map(p - e.yxy),
        map(p + e.yyx) - map(p - e.yyx)
    ));
}

void main() {
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Ray origin moves forward constantly to fly through the endless arches
    vec3 ro = uEyePos;
    ro.z -= uTime * 0.2; 

    vec3 rd = normalize(uCamForward + uCamRight * uv.x + uCamUp * uv.y);

    float t = 0.0;
    int max_steps = 100;
    float d = 0.0;
    vec3 p;
    
    // Raymarching loop
    for(int i = 0; i < max_steps; i++) {
        p = ro + rd * t;
        d = map(p);
        if(d < 0.001 || t > 40.0) break;
        t += d * 0.8; // Multiply by 0.8 to prevent artifacts on complex surfaces
    }

    vec3 col = vec3(0.0);

    if(t < 40.0) {
        // We hit the architecture! 
        vec3 n = getNormal(p);
        
        // Base color based on the normal vectors to emphasize the curved geometry
        vec3 surfaceColor = 0.5 + 0.5 * cos(uTime * 0.5 + p.y * 0.5 + vec3(0, 2, 4));
        
        // Fake lighting: Darker in the crevices, brighter on the edges
        float lighting = max(0.0, dot(n, normalize(vec3(1.0, 1.0, -1.0))));
        float ambient = 0.2;
        
        // Depth-based fog (fades to a dark color in the distance)
        float fog = exp(-t * t * 0.005);
        
        // Overdrive the edge colors to trigger your Bloom effect!
        float edgeGlow = smoothstep(0.4, 0.6, lighting) * 1.5;
        
        col = surfaceColor * (lighting + ambient);
        col += surfaceColor * edgeGlow; 
        col *= fog;
    }
    
    FragColor = vec4(col, 1.0);
}