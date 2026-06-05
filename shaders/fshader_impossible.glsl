#version 150

in vec3 fragPos;
in vec4 fragColor;
in float vScreenY;
in float vScreenX;

uniform vec3  uLightPos;
uniform vec3  uLightColor;   // per-slot light tint (Tier 2); white = neutral
uniform vec3  uEyePos;
uniform float uTime;        // for animating the gradient
uniform float uObjHeight;   // world-space height of the object (for normalizing Y)
uniform float uLockGlow;    // 0..1, brightness pulse when figure clicks into solved pose
uniform float uPostSolveTime; // seconds since the figure LOCKED (0 while unsolved)
uniform int   uIsBall;      // 1 = render as bright emissive ball, 0 = normal figure

out vec4 outColor;

// ─── 3D simplex noise — vendored from stegu/webgl-noise (MIT) ────────────────
// Ian McEwan, Ashima Arts; maint. Stefan Gustavson.
// https://github.com/stegu/webgl-noise — see shaders/third_party/{noise3D.glsl,LICENSE}.
// Used to drive the post-solve "object ignites into fire" effect. Inlined
// because InitShader() loads a single file with no #include support.
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

// fbm — a few octaves of the simplex noise above, the standard fire driver.
float fireFbm(vec3 p)
{
    return 0.55 * snoise(p)
         + 0.28 * snoise(p * 2.1 + 17.0)
         + 0.16 * snoise(p * 4.3 + 41.0);
}

// 2D fbm of the simplex noise above — rock grain / micro-detail and domain warp.
float fbm2(vec2 p)
{
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; i++) { s += a * snoise(vec3(p, 0.0)); p *= 2.03; a *= 0.5; }
    return s;
}

// ─── Voronoi cell-border distance (Inigo Quilez, iquilezles.org/articles/voronoilines) ─
// The cracked-rock look needs PLATES (Voronoi cells) separated by clean CRACK
// lines (cell borders). Two passes: find the nearest cell point, then measure the
// minimum distance to the borders with the surrounding cells — giving a crisp,
// continuous edge network (impossible with plain ridged noise). Evaluated on the
// seam-safe rp coordinate so the plates/cracks match across the magic join.
vec2 vhash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}
// returns vec2( distanceToNearestBorder , per-cell random id )
vec2 voronoiBorder(vec2 x)
{
    vec2 n = floor(x);
    vec2 f = fract(x);
    vec2 mr = vec2(0.0);
    vec2 mg = vec2(0.0);
    float md = 8.0;
    for (int j = -1; j <= 1; j++)
    for (int i = -1; i <= 1; i++)
    {
        vec2 g = vec2(float(i), float(j));
        vec2 o = vhash2(n + g);
        vec2 r = g + o - f;
        float d = dot(r, r);
        if (d < md) { md = d; mr = r; mg = g; }
    }
    float border = 8.0;
    for (int j = -2; j <= 2; j++)
    for (int i = -2; i <= 2; i++)
    {
        vec2 g = mg + vec2(float(i), float(j));
        vec2 o = vhash2(n + g);
        vec2 r = g + o - f;
        vec2 dr = r - mr;
        if (dot(dr, dr) > 1e-5)
            border = min(border, dot(0.5 * (mr + r), normalize(dr)));
    }
    float cellId = fract(sin(dot(n + mg, vec2(12.9898, 78.233))) * 43758.5453);
    return vec2(border, cellId);
}

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
    // Per-slot light COLOR (Tier 2) tints the diffuse + specular response.
    // Ambient stays neutral so shadowed faces keep the figure's own color.
    // uLightColor is one global value per draw, so it shifts every fragment
    // identically and never reintroduces a step at the solved seam.
    vec3 litColor = base * (ambient + diffuse * uLightColor) * brightness
                  + uLightColor * specular;

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

    // ── Warm-slot enrichment (warmth-gated; 0 for every neutral/white slot) ──
    //   warmth = R - B of uLightColor → 0 for white/neutral slots, ~0.5 for the
    //   amber "forge" arch. Defined ONCE here and reused by the solve payoff
    //   below. White-lit slots therefore reduce EXACTLY to the prior behavior —
    //   seam-safe, zero regression — while the forge arch gains hot-metal life.
    float warmth = clamp(uLightColor.r - uLightColor.b, 0.0, 1.0);

    // (a) RIM LIGHT — warm glow just inside the silhouette, so the hot iron
    // catches light along its turning edges and reads against the dark forge
    // backdrop. `facing` is small where the face turns away from the viewer;
    // this band is broader than the dark outline (which uses 0.30→0.05), so it
    // sits just inside it as edge-lit metal rather than fighting the outline.
    float rim = smoothstep(0.55, 0.08, facing);
    litColor += uLightColor * warmth * rim * 0.45;

    // (b) SHEEN — a tighter, brighter specular hot-spot only for warm slots, so
    // the forged surface glints under the forge key light. Neutral slots skip
    // it entirely (warmth = 0).
    float sheen = pow(max(dot(R, V), 0.0), 120.0) * warmth * 0.9;
    litColor += uLightColor * sheen;

    // ── Solve payoff ────────────────────────────────────────────────────────
    // Two themed additive layers, both GATED by the same `warmth`. White-lit
    // slots reduce EXACTLY to the old flat white pulse with no afterglow.

    // (1) STRIKE — the existing lock pulse, recoloured toward white-hot. For a
    // neutral slot mix(white,white,·)=white, so this is identical to the old
    // `+= vec3(uLockGlow)`. For the forge it blooms amber → white at the peak.
    vec3 strike = mix(uLightColor, vec3(1.0), clamp(uLockGlow * 1.5, 0.0, 1.0))
                * uLockGlow;
    litColor += strike;

    // (2) GENERATIVE ROCK + LAVA — Tier-2 effect on the Impossible Arch ────────
    // The arch is rendered as cracked basalt with glowing lava in the crevices:
    //   • UNSOLVED: pure stone, no glow — the puzzle is a rock arch to rotate.
    //   • ON LOCK:  the LAVA REVEAL — a clock-sweep traces the loop loading
    //               lava into the cracks (plays ONCE, ~7s).
    //   • SETTLED:  rock pattern rotates gently CCW, lava noise drifts CCW in
    //               the cracks, and a warm bloom continuously sweeps the loop
    //               (the reveal's lead glow continued at slightly lower amp).
    // Gated for zero regression on every neutral-lit slot — `warmth` is ~0.52
    // on the amber-lit arch, ~0 on the white-lit rest, so this whole block is
    // a no-op for everything except slot 4.
    if (warmth > 0.001)
    {
        const float PI = 3.14159265;

        // Centred screen coordinate — seam-safe (both halves of an aligned join
        // share the same screen position, so anything keyed off this matches
        // across the magic seam with no step).
        vec2  d     = vec2(vScreenX - 0.5, vScreenY - 0.5);

        // Position AROUND the loop = screen-space angle about the figure centre.
        // A clock hand at 0..1 as it sweeps once around the loop; the small sine
        // adds a ragged edge so the advancing front isn't a clean ruled line.
        float angle = atan(d.y, d.x) / (2.0 * PI) + 0.5;
        angle += 0.035 * sin(vScreenX * 26.0 + vScreenY * 19.0);

        // ── REVEAL TIMELINE ─────────────────────────────────────────────────
        // After lock, wait LAVA_DELAY seconds (bare rock), then a clock-hand
        // sweep loads the lava angularly around the loop over LAVA_TRACE
        // seconds. Past 1.10 the sweep has fully wrapped → every crack glows.
        const float LAVA_DELAY = 0.7;
        const float LAVA_TRACE = 6.0;
        float lavaT       = max(uPostSolveTime - LAVA_DELAY, 0.0);
        float lavaActive  = step(0.001, lavaT);   // 0 until the trace begins (kills the branch-cut sliver)
        float lavaFront   = lavaT / LAVA_TRACE * 1.10;
        float traceW      = 0.13;
        float lavaReveal  = lavaActive * (1.0 - smoothstep(lavaFront - traceW, lavaFront + traceW, angle));

        {
            // ── GENERATIVE ROCK + LAVA ──────────────────────────────────────
            // Everything below samples the seam-safe rp coordinate (function of
            // face normal + screen position) so the plates, cracks, relief and
            // flow all match across the magic join.
            // ── 3D-ANCHORED FLOW ─────────────────────────────────────────────
            // The flames must read as licking ALONG each beam's real surface, not
            // sliding flatly across the screen silhouette. Everything below is a
            // function of (face normal, screen position) ONLY — which is seam-safe
            // because the two faces that meet at the magic join SHARE a normal AND
            // share screen position (see the Phong note above), so any such
            // function matches across the join with no step.
            //
            //  (1) FORESHORTEN per face. A beam turned edge-on to the viewer should
            //      show its flame texture squashed along the direction it recedes —
            //      exactly like a texture mapped onto the tilted face. `facing`
            //      (|N·V|) is the slant; the screen-space recede axis is how view
            //      depth changes per screen pixel, read straight off the world-
            //      position derivatives (dot(dx,V),dot(dy,V)) — no view basis
            //      needed. dx,dy are per-face constant and, under this fixed
            //      orthographic view, depend only on N, so they match at the join.
            vec2  fa    = vec2(dot(dx, V), dot(dy, V));     // screen recede direction
            float flen  = length(fa);
            vec2  axis  = flen > 1e-4 ? fa / flen : vec2(1.0, 0.0);
            float slant = clamp(facing, 0.40, 1.0);         // 1=face-on … small=edge-on

            //  (2) ROCK DRIFT — a slow continuous rotation of the rock sample coord
            //      around the figure centre. On each beam's silhouette this maps to
            //      flow along the beam's length in the CCW direction (top→left,
            //      left→down, bottom→right, right→up — matching the arrows). It's
            //      a smooth curl field, so no shear streaks at corners. The ball
            //      below is the prominent direction marker; this rotation gives the
            //      rock itself a continuous surface-following motion underneath.
            //      Slow enough to read as a quiet current, not a swirl.
            float spin = uPostSolveTime * 0.022;
            float csR = cos(spin), snR = sin(spin);
            vec2  rd  = vec2(d.x * csR + d.y * snR,
                            -d.x * snR + d.y * csR);

            // Stretch the sample coordinate along the recede axis (1/slant) so the
            // flame texture appears COMPRESSED along that axis on the slanted face.
            // Gentle foreshortening — enough to read as 3D-mapped, but scaled
            // down (×0.55) so the most edge-on faces don't comb into hard streaks.
            float al  = dot(rd, axis);
            float fs  = (1.0 / slant - 1.0) * 0.55;
            vec2  rp  = rd + axis * (al * fs);

            // ── GENERATIVE ROCK + SLOW LAVA (procedural, seam-safe) ──────────
            // Built the way generative-rock shaders do it: domain-warped VORONOI
            // gives broken PLATES (cells) separated by clean CRACK lines (cell
            // borders); FBM adds grain inside the plates; a fake normal map gives
            // real relief. The SLOW LAVA lives only in the crack network and is the
            // one thing that moves — a gentle creep, not the churn that read as
            // cheap. Everything samples the seam-safe rp coordinate (a function of
            // N + screen pos), so plates, cracks and relief all match across the
            // magic join. (No UV texture — its UVs differ across the join and would
            // crack the seam.)

            // (1) DOMAIN WARP (Quilez) — push the rock coordinate by an fbm offset
            //     so plates are organic and the grain gets geological striation,
            //     instead of a regular Voronoi lattice.
            vec2 q    = rp * 5.5;
            vec2 warp = vec2(fbm2(q + vec2(0.0, 0.0)),
                             fbm2(q + vec2(4.7, 1.3)));
            vec2 rc   = q + 0.55 * warp;

            // (2) VORONOI plates + crack borders — STATIC rock structure (no time).
            vec2  vor    = voronoiBorder(rc);
            float border = vor.x;     // ~0 on cracks, large in plate interiors
            float cellId = vor.y;     // per-plate random tone

            // (3) ROCK GRAIN — fbm micro-detail inside the plates (static).
            float grain  = 0.5 + 0.5 * fbm2(rp * 17.0);
            float grain2 = 0.5 + 0.5 * fbm2(rp * 38.0);

            // (4) CRACK NETWORK — a thin glowing line exactly on the cell border,
            //     plus a narrow warm halo hugging it. Rock dominates everywhere else.
            float crackCore = 1.0 - smoothstep(0.0, 0.045, border);  // thin hot line
            float crackGlow = 1.0 - smoothstep(0.0, 0.160, border);  // warm halo

            // (5) SLOW LAVA + AMBIENT CIRCULATION — once the trace has loaded, the
            //     molten brightness gently CREEPS around the loop, following each
            //     beam's surface direction, so the solved figure reads as a quiet
            //     current running around the impossible loop. The loop tangent in
            //     screen space is perpendicular to the radius from the figure
            //     centre; advecting the lava-flow noise along that unit tangent
            //     (CCW) is the circulation, plus a tiny in-place evolution so it
            //     shimmers as it drifts. All a function of (screen pos, N, time),
            //     so it's seam-safe; speed kept low — a silent ambient drift, not
            //     a churn. (The rock STRUCTURE stays perfectly static — only the
            //     glow within the cracks moves.)
            // Lava advection uses the smooth unit tangent — only visible inside the
            // crack network, so the small near-centre singularity stays hidden.
            // "−" sign on the offset sends features in the +tangent direction =
            // CCW around the loop (top→left, right→up, bottom→right, left→down),
            // matching the rock rotation above and the arrows the user drew.
            float rr        = length(d);
            vec2  lavaTan   = rr > 1e-4 ? vec2(-d.y, d.x) / rr : vec2(0.0);
            vec2  lavaCoord = rc * 1.6 - lavaTan * (uTime * 0.055);
            float lavaFlow  = 0.60 + 0.40 * (0.5 + 0.5 * snoise(vec3(lavaCoord, uTime * 0.045)));

            // FLOW PULSE — two opposing brightness crests travel CCW around the
            // loop with the rock+lava drift, so the molten brightness has a
            // subtle directional rhythm. Function of (angle, time) only, so
            // seam-safe; ±18% amplitude — visible motion, not strobing.
            float flowPhase = angle * 2.0 + uTime * 0.95;
            float flowPulse = 0.88 + 0.18 * sin(flowPhase);

            // The cracks only glow molten where the circular load has PASSED;
            // ahead of the ring they stay cold dark grooves in the rock. The flow
            // pulse modulates the heat once revealed, so brighter molten patches
            // travel around the loop with the rock+lava drift.
            float heatRaw = (crackCore * 0.90 + crackGlow * 0.35) * lavaFlow;
            float heat    = clamp(heatRaw * lavaReveal * flowPulse, 0.0, 1.0);
            float molten  = smoothstep(0.05, 0.34, heat);  // 0 = grey rock … 1 = lava crack

            // ── RELIEF (procedural normal map, Mikkelsen surface gradient) ───
            // Plates stand proud, cracks carve DEEP, grain adds micro-roughness.
            // Pure fragment-shader bump — geometry/silhouette/join untouched, and
            // Nb stays a function of (N, screen pos) so it matches at the join.
            float h   = smoothstep(0.0, 0.22, border)          // plate body raised
                      + 0.12 * grain + 0.06 * grain2           // rocky micro-relief
                      - 0.90 * crackGlow;                      // crevices sink
            float dHx = dFdx(h);
            float dHy = dFdy(h);
            vec3  br1 = cross(dy, N);
            vec3  br2 = cross(N, dx);
            float det = dot(dx, br1);
            vec3  sgrad = sign(det) * (dHx * br1 + dHy * br2);
            vec3  Nb  = normalize(abs(det) * N - 6.5 * sgrad);   // bump strength

            float diffB  = max(dot(Nb, L), 0.0);
            float shadeB = clamp(0.42 + diffB * 0.78, 0.0, 1.0);  // soft contrast, lifted floor
            float specB  = pow(max(dot(reflect(-L, Nb), V), 0.0), 20.0);

            // ── ALBEDO ───────────────────────────────────────────────────────
            // Dark GREY basalt: per-plate tone (cellId) + grain mottle, kept neutral.
            float tone = mix(0.78, 1.18, cellId) * (0.82 + 0.18 * grain);
            vec3  rock = mix(vec3(0.105, 0.102, 0.110),
                             vec3(0.300, 0.293, 0.302), grain) * tone;
            // Lava ember ramp — toned down, no white-hot.
            vec3  hot  = mix(vec3(0.42, 0.07, 0.02), vec3(0.92, 0.32, 0.05),
                             smoothstep(0.00, 0.50, heat));
            hot = mix(hot, vec3(1.00, 0.60, 0.18), smoothstep(0.50, 0.82, heat));
            hot = mix(hot, vec3(1.00, 0.78, 0.42), smoothstep(0.88, 1.00, heat));
            // DIRECTIONAL TEMPERATURE — the ember tone rides the same flowPhase as
            // the pulse, so one stretch of the loop reads as "fresh hot" (yellower)
            // and the trailing stretch as "cooling" (deeper red). A gentle gradient
            // that travels with the flow, making the loop's direction unmistakable
            // and tying the colour cue to the rock+lava motion.
            float tempCycle = 0.5 + 0.5 * sin(flowPhase);
            vec3  tempBias  = mix(vec3(0.95, 0.86, 0.86), vec3(1.05, 1.06, 1.02), tempCycle);
            hot *= tempBias;

            // ── AMBIENT FIRELIGHT — gentle breathing flicker so the lava feels
            // alive and the rock near cracks picks up a soft warm bounce, as if
            // lit by the molten light. Two detuned sines for an irregular
            // not-quite-periodic pulse; magnitude small ("silent" ambient).
            // Function of uTime only, so it's perfectly seam-safe.
            float flicker = 0.86 + 0.14 * (0.5 * sin(uTime * 0.41)
                                         + 0.5 * sin(uTime * 0.93 + 1.7));

            // ── MATERIAL SPLIT — matte lit rock vs. emissive slow lava ───────
            // Rock gets a tiny warm bounce gated by crackGlow (only rock NEAR
            // cracks picks up firelight) and by lavaActive (no bounce before solve).
            vec3  warmBounce = vec3(0.55, 0.18, 0.05) * crackGlow * (1.0 - molten)
                             * flicker * lavaActive * 0.18;
            vec3  litRock = rock * shadeB
                          + vec3(0.80, 0.78, 0.80) * specB * (1.0 - molten) * 0.10
                          + warmBounce;
            vec3  glow    = hot * (0.50 + 0.30 * heat) * flicker;
            vec3  lava    = mix(litRock, glow, molten);

            // Re-assert the dark edge lines so face boundaries stay legible.
            lava *= (1.0 - 0.55 * edge);
            litColor = lava;
        }

        // ── REVEAL FRONT — a bright warm bloom riding the clock-sweep that
        // loads the lava during the reveal phase. Fades out as the sweep
        // completes (lavaFront passes 1.05).
        float lead       = exp(-pow((angle - lavaFront) / (traceW * 1.2), 2.0));
        float leadActive = (1.0 - smoothstep(0.80, 1.05, lavaFront)) * step(0.001, lavaT);
        litColor += vec3(1.0, 0.66, 0.28) * lead * leadActive * warmth * 1.3;

        // ── AMBIENT SWEEPING LIGHT — almost the same as the reveal's lead glow,
        // just continuous: same warm bloom colour, same falloff width, same
        // intensity (a touch less so it doesn't read as the reveal animation
        // re-firing). Kicks in once the reveal sweep completes and keeps
        // sweeping CCW around the loop indefinitely as the ongoing ambient.
        // Angular wrap-around handled so the bloom stays continuous across the
        // branch cut.
        float ambFront  = mod(lavaT * 0.07, 1.0);              // wraps 0→1 every ~14s
        float angDiff   = abs(angle - ambFront);
        angDiff         = min(angDiff, 1.0 - angDiff);         // shortest distance, wrap-safe
        float ambLead   = exp(-pow(angDiff / (traceW * 1.2), 2.0));   // SAME width as reveal lead
        float ambActive = smoothstep(1.0, 1.20, lavaFront);    // kicks in once reveal is done
        litColor += vec3(1.0, 0.66, 0.28) * ambLead * ambActive * warmth * 1.15;
    }

    litColor  = min(litColor, vec3(1.0));

    outColor = vec4(litColor, fragColor.a);
}