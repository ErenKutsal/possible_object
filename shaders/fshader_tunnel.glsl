#version 330 core

in vec2 fragUV; // Assuming your vshader_quad.glsl passes UVs 0 to 1
out vec4 FragColor;

uniform vec3 uEyePos;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform vec3 uCamForward;
uniform float uTime;
uniform float uAspect;

// 2D Rotation matrix for twisting the tunnel
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Signed Distance Function (SDF) 
float map(vec3 p) {
    vec3 q = p;
    
    // Twist the tunnel over the Z axis and animate the twist over time
    q.xy *= rot(q.z * 0.15 + uTime * 0.2); 
    
    // An infinite cylinder of radius 2.0. 
    // We invert it (2.0 - length) so we are inside looking at the walls
    float tunnel = 2.0 - length(q.xy); 
    
    // Add organic/sci-fi displacement (ribs and bumps)
    float bumps = sin(q.z * 5.0) * cos(q.x * 4.0) * sin(q.y * 4.0) * 0.2;
    
    return tunnel + bumps;
}

void main() {
    // Map UVs from [0, 1] to NDC [-1, 1] and correct the aspect ratio
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Ignore the true X and Y from the C++ camera so we don't spawn in a wall!
    // Force the camera to be perfectly centered in the tunnel at X=0, Y=0
    vec3 ro = vec3(0.0, 0.0, uEyePos.z); 
    
    // Fly forward
    ro.z -= uTime * 1.0;

    // Set up the Ray Direction using the vectors passed from your C++ code
    vec3 rd = normalize(uCamForward + uCamRight * uv.x + uCamUp * uv.y);

    // Raymarching Loop
    float t = 0.0;
    int max_steps = 64;
    float d = 0.0;
    vec3 p;
    
    for(int i = 0; i < max_steps; i++) {
        p = ro + rd * t;
        d = map(p); // Check distance to the tunnel walls
        if(d < 0.01 || t > 50.0) break; // Hit the wall, or reached max draw distance
        t += d;     // Step forward by the safe distance
    }

    vec3 col = vec3(0.0); // Default to black/darkness

    if(t < 50.0) {
        // We hit the wall! Calculate fake lighting and colors
        
        // Depth-based fog/glow (fades to black in the distance)
        float glow = exp(-t * 0.08); 
        
        // Dynamic Cyberpunk/Sci-Fi Colors mapped to the Z coordinate
        vec3 colorA = vec3(0.1, 0.8, 1.0); // Cyan
        vec3 colorB = vec3(0.9, 0.1, 0.8); // Magenta
        vec3 baseColor = mix(colorA, colorB, sin(p.z * 0.5 + uTime) * 0.5 + 0.5);
        
        // Add fast-moving light rings to sell the illusion of speed
        float rings = smoothstep(0.0, 0.2, sin(p.z * 4.0 - uTime * 3.0));
        
        // Combine everything (pushing values > 1.0 will trigger your Bloom!)
        col = baseColor * glow * (1.0 + rings * 1.0);
    }
    
    FragColor = vec4(col, 1.0);
}