#version 150

// Cyber Grid Holographic background — fades in once the Penrose Triangle
// is solved. Uses the fullscreen-quad vertex shader (vshader_halo.glsl).
//
// Features:
//   1. Deep cyber-space indigo void backdrop.
//   2. Volumetric neon/aurora fog near the horizon.
//   3. Analytical perspective-scrolling neon floor and ceiling grids.
//   4. Raymarched floating holographic wireframe boxes that sway and rotate in 3D.
//   5. Chromatic aberration and scanline retro-projection filter.
//   6. uAmount fades everything in from the pale-slate neutral colour.

in vec2 vNdc;
out vec4 outColor;

uniform vec3  uBaseColor;
uniform float uTime;
uniform float uAmount;

// Minimal simplex noise for organic sways and rotations
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+10.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v)
{
    const vec2  C = vec2(1.0/6.0, 1.0/3.0);
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 =   v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
               i.z + vec4(0.0, i1.z, i2.z, 1.0))
             + i.y + vec4(0.0, i1.y, i2.y, 1.0))
             + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// ── 3D Rotation matrices ─────────────────────────────────────────────────────
mat2 rot2D(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

// ── Wireframe Box Frame SDF ──────────────────────────────────────────────────
float sdBoxFrame(vec3 p, vec3 b, float e)
{
    p = abs(p) - b;
    vec3 q = abs(p + e) - e;
    return min(min(
        length(max(vec3(p.x, q.y, q.z), 0.0)) + min(max(p.x, max(q.y, q.z)), 0.0),
        length(max(vec3(q.x, p.y, q.z), 0.0)) + min(max(q.x, max(p.y, q.z)), 0.0)),
        length(max(vec3(q.x, q.y, p.z), 0.0)) + min(max(q.x, max(q.y, p.z)), 0.0));
}

// ── Map the floating wireframe boxes ─────────────────────────────────────────
float mapCubes(vec3 p, out vec3 cellId)
{
    // Infinite grid replication of cubes
    vec3 spacing = vec3(6.0, 6.0, 6.0);
    
    // Slow drift upwards and sway
    p.y -= uTime * 0.4;
    p.x += sin(p.y * 0.3 + uTime * 0.5) * 0.6;
    p.z += cos(p.y * 0.2 - uTime * 0.3) * 0.6;
    
    vec3 q = p;
    cellId = floor(q / spacing); // approximate cell id for unique rotation/color
    q = mod(q, spacing) - spacing * 0.5;
    
    // Rotate each cell individually
    float angleX = uTime * 0.3 + snoise(cellId * 1.5) * 6.28;
    float angleY = uTime * 0.5 + snoise(cellId * 2.3 + vec3(1.0)) * 6.28;
    float angleZ = uTime * 0.2;
    
    q.yz = rot2D(angleX) * q.yz;
    q.xz = rot2D(angleY) * q.xz;
    q.xy = rot2D(angleZ) * q.xy;
    
    // Draw hollow wireframe box frame
    return sdBoxFrame(q, vec3(0.7), 0.06);
}

// Get color for a specific cell index
vec3 getCellColor(vec3 id)
{
    float val = fract(sin(dot(id, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
    // Cycle between electric pink, neon cyan, purple
    if (val < 0.33) return vec3(0.0, 1.0, 1.0);       // Cyan
    if (val < 0.66) return vec3(1.0, 0.0, 0.7);       // Magenta
    return vec3(0.6, 0.0, 1.0);                      // Violet
}

void main()
{
    vec3 finalColor = uBaseColor;

    if (uAmount > 0.001)
    {
        // ── 1. Cyber indigo space void backdrop ──────────────────────────────
        vec3 voidColor = mix(vec3(0.01, 0.01, 0.04), vec3(0.04, 0.02, 0.10), length(vNdc) * 0.6);
        
        // ── 2. Camera setup with slow majestic drift ─────────────────────────
        vec2 uv = vNdc;
        vec3 ro = vec3(0.0, 0.0, -6.0);
        ro.x += sin(uTime * 0.08) * 0.4;
        ro.y += cos(uTime * 0.12) * 0.3;
        
        // Ray direction
        vec3 rd = normalize(vec3(uv * vec2(1.6, 1.0), 1.6));
        // Rotate camera slightly over time
        rd.xz = rot2D(sin(uTime * 0.05) * 0.08) * rd.xz;
        rd.yz = rot2D(cos(uTime * 0.03) * 0.04) * rd.yz;
        
        vec3 col = voidColor;
        
        // ── 3. Horizon neon/aurora glow ──────────────────────────────────────
        float horizon = exp(-abs(rd.y - 0.05) * 8.0);
        vec3 cyberColor = mix(vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 0.8), 0.5 + 0.5 * sin(uTime * 0.3));
        col += cyberColor * horizon * 0.35;
        
        // ── 4. Analytical Floor and Ceiling Grids ────────────────────────────
        // Floor at y = -2.8, Ceiling at y = 2.8
        for (int side = -1; side <= 1; side += 2)
        {
            float limitY = 2.8 * float(side);
            float tGrid = (limitY - ro.y) / rd.y;
            if (tGrid > 0.0)
            {
                vec3 intersect = ro + rd * tGrid;
                // Fade out grid lines in the distance
                float fog = exp(-tGrid * 0.12);
                if (fog > 0.01)
                {
                    vec2 gridUv = intersect.xz;
                    gridUv.y += uTime * 0.8 * float(side); // scroll forward/backward
                    
                    // Sharp grid lines using smoothstep
                    float lx = smoothstep(0.05, 0.0, abs(fract(gridUv.x) - 0.5));
                    float ly = smoothstep(0.05, 0.0, abs(fract(gridUv.y) - 0.5));
                    float gridLines = max(lx, ly);
                    
                    // Soft neon grid glow
                    float gx = smoothstep(0.3, 0.0, abs(fract(gridUv.x) - 0.5));
                    float gy = smoothstep(0.3, 0.0, abs(fract(gridUv.y) - 0.5));
                    float gridGlow = max(gx, gy) * 0.35;
                    
                    vec3 gridColor = (side == -1) ? vec3(0.0, 0.8, 1.0) : vec3(1.0, 0.0, 0.6);
                    col += (gridColor * gridLines * 0.9 + gridColor * gridGlow) * fog;
                }
            }
        }
        
        // ── 5. Raymarched Holographic Floating Boxes ──────────────────────────
        float t = 0.1;
        float glowAccum = 0.0;
        vec3 glowCol = vec3(0.0);
        
        for (int i = 0; i < 35; i++)
        {
            vec3 p = ro + rd * t;
            vec3 cellId;
            float d = mapCubes(p, cellId);
            
            // Volumetric glow calculation
            float g = exp(-abs(d) * 6.5);
            glowAccum += g;
            glowCol += g * getCellColor(cellId);
            
            // Step forward
            t += max(abs(d) * 0.65, 0.06);
            if (t > 22.0) break;
        }
        
        // Add glowing holographic boxes to the scene
        col += glowCol * 0.28 * exp(-t * 0.03);
        
        // ── 6. Retro scanline and vignette overlay ───────────────────────────
        float scanline = 0.95 + 0.05 * sin(uv.y * 450.0 + uTime * 10.0);
        col *= scanline;
        
        // Vignette
        float vig = 1.0 - dot(vNdc, vNdc) * 0.32;
        col *= clamp(vig, 0.0, 1.0);
        
        // Blend from the slate-gray background to the final cyber rendering
        finalColor = mix(uBaseColor, col, uAmount);
    }

    outColor = vec4(finalColor, 1.0);
}
