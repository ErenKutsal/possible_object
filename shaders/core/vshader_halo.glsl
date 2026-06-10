#version 150
// Used by: arch.cpp, arch_proc.cpp, neckercube.cpp, penrose.cpp, penrose_blocks.cpp, penrose_blocks_proc.cpp, reutersvard.cpp | Object: Background pre-pass (radial bloom / halo / lava / forge-glow) | Effect: Volumetric ambient halo glow vertex transform


// Firelight halo — fullscreen quad vertex shader.
// Slot 4 (Impossible Arch) only. Draws a soft radial warm glow on the
// background, centred on the figure, so the molten lava visibly LIGHTS the
// scene around it instead of the figure being pasted onto a flat backdrop.
//
// The quad is emitted at NDC z = 0.999 so the figure (which draws after, with
// depth testing) always overwrites it where geometry is present. We don't need
// to disable depth testing for the halo pass — the depth value just sits at
// the far edge of the buffer.

in vec2 aPos;        // -1..1 fullscreen-quad corners
out vec2 vNdc;       // -1..1 passed through for the gaussian falloff

void main()
{
    gl_Position = vec4(aPos, 0.999, 1.0);   // depth 0.999 → behind the figure
    vNdc        = aPos;
}
