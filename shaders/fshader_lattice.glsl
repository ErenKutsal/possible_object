#version 330 core

in vec2 fragUV;
out vec4 FragColor;

uniform vec3 uEyePos;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform vec3 uCamForward;
uniform float uTime;
uniform float uAspect;

// Standard Signed Distance Field (SDF) functions
float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// SDF for a "Hollowed out" repeating grid lattice (TRON style)
float map(vec3 p) {
    // 1. MODULO RELATIVITY FIX: Ignore the camera's true X/Y so we are always centered in a cell
    vec3 ro_local = vec3(0.0, 0.0, uEyePos.z);
    
    // 2. Slow down the rotation math for the entire structure
    float slowTime = uTime * 0.1;
    float s = sin(slowTime), c = cos(slowTime);
    mat2 rotY = mat2(c, -s, s, c);
    
    // Rotate world space slowly
    p.xz *= rotY;

    // Continuous forward flight
    p.z -= uTime * 0.5;

    // 3. INFINITE GRID MODULO
    float spacing = 5.0; // Size of each infinite grid room
    // Repeat the math for space modulo 'spacing', centered around 0.0
    vec3 c3 = vec3(spacing);
    vec3 q = mod(p + c3 * 0.5, c3) - c3 * 0.5; 

    // Create a cage effect (distance to edges of a box frame)
    float thickness = 0.015; // Base thickness
    // Make thickness pulsate Organically
    thickness += 0.04 * sin(length(p)*0.3 + slowTime * 5.0);
    //thickness += 0.005 * sin(slowTime * 3.0);
    
    // Distance to standard box edges [1.0, 1.0, 1.0] in local modulo cell
    vec3 d = abs(q) - 1.0;
    float mc = min(d.x, min(d.y, d.z));
    float cageEdges = length(max(d,0.0)) + min(mc,0.0);

    // Make the standard solid box distance "Hollow" to create lines/pipes
    float cage_pipes = abs(cageEdges) - thickness;
    
    return cage_pipes; 
}

void main() {
    // Standard ray setup
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= uAspect;

    // Use mod relativity fix location
    vec3 ro = vec3(0.0, 0.0, uEyePos.z);
    ro.z -= uTime * 0.5; // Must sync with map() flight

    vec3 rd = normalize(uCamForward + uCamRight * uv.x + uCamUp * uv.y);

    // Raymarching Loop
    float t = 0.0;
    int max_steps = 128;
    float d = 0.0;
    vec3 p;
    for(int i = 0; i < max_steps; i++) {
        p = ro + rd * t;
        d = map(p);
        if(d < 0.001 || t > 60.0) break;
        t += d;
    }

    vec3 col = vec3(0.0); // DEFAULT TO COSMIC DUST/DARKNESS

    if(t < 60.0) {
        // We hit the geometric cage!
        
        // Depth-based fog (fade structure to black)
        float fog = 1.0 - smoothstep(10.0, 60.0, t);
        
        // Dynamic abstract geometric colors based on local Modulo cell (q) coordinates
        // Sync space rotation to map() logic
        float slowTime = uTime * 0.1;
        float s = sin(slowTime), c = cos(slowTime);
        mat2 rotY = mat2(c, -s, s, c);
        vec3 p_rot = vec3(p.x, p.y, p.z);
        p_rot.xz *= rotY; // reverse rotation to track colors to structure
        
        // Stretch the gradient over a massive distance (0.01) and slow the time shift
        float colorP = abs(sin(p_rot.z * 0.01 + p_rot.x * 0.01 + slowTime * 0.2));
        
        // Lower the base brightness and overdrive multiplier for a softer, stable glow
        vec3 colorA = vec3(0.1, 0.7, 1.0) * 1.2; // Soft Cyan
        vec3 colorB = vec3(1.0, 0.1, 0.7) * 1.2; // Soft Purple
        
        vec3 latticeColor = mix(colorA, colorB, colorP);
        
        col = latticeColor * fog;
    }
    
    FragColor = vec4(col, 1.0);
}