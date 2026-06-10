#version 150
// Used by: arch_proc.cpp | Object: Curved Impossible Arch | Effect: Forge-glow halo background pre-pass


// Firelight halo — fragment shader.
// Conceptually: the lava on the figure is a LIGHT SOURCE that warms the air
// around it. Implementation: paint the background pale slate, then add a soft
// gaussian warm spill centred on the figure's screen position. The amount
// uniform ramps up over the lava reveal so the surroundings visibly heat up
// as the molten cracks ignite, and tapers to a quiet hold while the ambient
// sweeping light keeps tracing the loop.
//
// Pedagogical note: this is the cheapest possible "object lights the scene"
// effect — one fullscreen additive falloff, no FBO, no IBL probe. The point
// is that the figure is no longer decoupled from its environment; light
// emitted by the figure visibly reaches the world it sits in.

in vec2 vNdc;        // -1..1, position on the fullscreen quad
out vec4 outColor;

uniform vec3  uBaseColor;     // the shared pale slate every other slot uses
uniform vec3  uHaloColor;     // warm amber matching the lava temperature
uniform vec2  uHaloCenter;    // figure's screen-space centre in NDC (-1..1)
uniform float uHaloAmount;    // 0..1 — fades in over the reveal, then holds
uniform float uHaloFalloff;   // gaussian sharpness; higher = tighter spill

void main()
{
    vec2  d    = vNdc - uHaloCenter;
    float r2   = dot(d, d);
    float halo = exp(-r2 * uHaloFalloff);
    outColor   = vec4(uBaseColor + uHaloColor * halo * uHaloAmount, 1.0);
}
