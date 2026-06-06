# Impossible Shapes — OpenGL Project Plan

*An interactive 3D illusion game showcasing the OpenGL pipeline through five themed levels in a magical forest.*

---

## 1. Vision

An interactive game inspired by impossible-shape optical illusions (Penrose triangle, Penrose stairs, Escher figures). The player explores a magical forest, discovering shrines that each hide an impossible figure. Each figure is built from physically disconnected 3D geometry — it only *looks* impossible from one specific viewing angle. The player rotates the camera to find that magic angle, locks the illusion, then traces the figure's path to complete the level.

Each of the five levels is a different elemental "shrine" within the same forest setting, and each one serves as a showcase for a different chunk of the OpenGL rendering pipeline. The result is a small, polished, visually distinct game that doubles as a portfolio piece for graphics programming.

---

## 2. Core Gameplay Mechanic

### The illusion

Every impossible figure in the game is rendered from three or more separate 3D meshes that do not touch in world space. The trick is that, from one specific camera position, the endpoints of these meshes align in screen space, creating the visual appearance of a connected impossible figure (the same effect used by the Penrose triangle sculpture in Perth, Australia).

### The state machine (per level)

```
ORBITING ── alignment > 0.99 for 0.5s ──▶ LOCKING ──animation done──▶ TRACING ──trace complete──▶ COMPLETED
    ▲                                                                                                │
    └─────── player exits trace (Esc) ──────────────────────────────────────────────────────────────┘
```

**ORBITING.** Player drags the mouse to orbit the camera around the figure's pivot. Each frame the renderer computes an **alignment score** in `[0, 1]`:

1. For each pair of anchor vertices that should visually meet, project both to NDC.
2. Compute the screen-space distance between the pair, normalized by viewport.
3. Aggregate across all pairs into a single float.

This continuous value drives all visual feedback — there is no "warmer/colder" UI bar. Instead:

- Rim glow intensity on the figure
- Particle attraction toward the anchor points
- Bloom strength
- Ambient audio pitch / volume

**LOCKING.** Once alignment exceeds threshold and holds for ~0.5s, camera input is disabled and the lock animation plays (~1.5s): vines tighten across the gaps, theme-appropriate VFX burst at the anchor points, a soft bloom flash, gentle camera dolly-in.

**TRACING.** The mouse cursor projects to a plane near the figure and snaps to the nearest figure edge. A verlet-rope trail follows the snap point with stiffness; the rope is rendered as a glowing tube via a geometry shader. The player traces the full closed loop in one continuous stroke. `distance covered / total perimeter` = progress.

**COMPLETED.** Final bloom burst, particles, "level complete" card, transition to next shrine.

---

## 3. Project Scope — Five Levels

Each level is a "shrine / clearing / phenomenon" within one continuous magical forest. Time-of-day progresses across levels (dawn → night) so the same forest pipeline produces dramatically different visuals each time.

| # | Theme | Time | OpenGL pipeline showcase | Puzzle figure |
|---|-------|------|--------------------------|---------------|
| 1 | **Floral** | Morning | Instanced foliage, vertex-shader wind, leaf SSS, volumetric god rays | Vined Penrose triangle |
| 2 | **Stone** | Afternoon, dappled | Tessellation, parallax occlusion mapping, PBR rough materials | Carved devil's fork on stone tablet |
| 3 | **Water** | Midday | Compute-shader height-field fluid sim, planar reflections, refraction, caustics | Penrose stairs as flowing water (Escher waterfall) |
| 4 | **Fire** | Sunset | Compute-shader particles, screen-space heat distortion, dynamic point lighting | Forged iron Penrose triangle in flames |
| 5 | **Cosmic** | Deep night | Screen-space gravitational lensing, raymarched volumetrics, extreme HDR + bloom, deferred composition | Light bending around a singularity into a Penrose loop |

**Color/movement variety check:** green/gold → grey/moss → blue/silver → orange/black → purple/deep blue. Movement: gentle → static → liquid → kinetic → slow surreal warp. Every level looks distinct in a single screenshot.

---

## 4. OpenGL Pipeline Showcase Matrix

Each level introduces techniques the previous ones don't. No level repeats another level's headline trick.

| Technique | Floral | Stone | Water | Fire | Cosmic |
|---|:---:|:---:|:---:|:---:|:---:|
| Deferred shading (G-buffer) | ✓ | ✓ | ✓ | ✓ | ✓ |
| PBR + IBL from HDRI | ✓ | ✓ | ✓ | ✓ | ✓ |
| Cascaded shadow maps | ✓ | ✓ | ✓ | ✓ | ✓ |
| Bloom + ACES tone mapping + LUT | ✓ | ✓ | ✓ | ✓ | ✓ |
| GPU instancing (foliage) | ★ | | | | |
| Vertex-shader wind animation | ★ | | | | |
| Alpha-tested foliage + leaf SSS | ★ | | | | |
| Volumetric god rays (raymarched) | ★ | | | | |
| Tessellation shader | | ★ | | | |
| Parallax occlusion mapping | | ★ | | | |
| Compute-shader height-field fluid | | | ★ | | |
| Planar reflections (render-to-texture) | | | ★ | | |
| Refraction (framebuffer sampling) | | | ★ | | |
| Projected caustics | | | ★ | | |
| Compute-shader particle system | | | | ★ | |
| Screen-space heat distortion | | | | ★ | |
| Many dynamic point lights | | | | ★ | |
| Screen-space gravitational lensing | | | | | ★ |
| Raymarched volumetric fog | | | | | ★ |

★ = headline feature of that level. ✓ = shared engine feature used across all levels.

---

## 5. Game Design

### Application flow

```
TitleScreen ──▶ ShrineSelect ──▶ Loading ──▶ Level (state machine above) ──▶ LevelComplete ──▶ next or ShrineSelect
                                                  ▲
                                                  ▼
                                                Pause overlay (Esc)
```

### Title screen
Game logo over a softly rotating impossible figure (live, low-detail floral figure). Three buttons: *Begin Journey*, *Shrine Select*, *Settings*. Quiet forest ambience. Subtle parallax on mouse move.

### Shrine select
Five cards horizontally arranged, each a thumbnail with the shrine's name. Sequential unlock OR all open from start.

### Loading screen
A tip + soft particle animation. Loads the level's HDRI, prefilters the IBL cubemaps (this takes a moment), loads textures and meshes.

### In-game HUD
Minimal. A subtle compass-ring at the edge of the screen that warms in color as alignment increases — or no HUD at all and let the figure itself communicate. Pause (Esc) opens an overlay: *Resume / Restart / Shrine Select / Quit*.

### Level complete card
Black fade, name of next shrine, two seconds, auto-advance or return to select.

### UI implementation
**Dear ImGui (docking branch)** with a custom theme (warm greys, serif font for titles). Saves about a week vs. building a custom UI system.

---

## 6. Asset Pipeline

### Sources (cite all in the final writeup)

| Source | Used for | License |
|---|---|---|
| **Blender + Cycles** | Pre-rendered HDRI backdrops; custom meshes (puzzle figures, vines) | GPL, free |
| **Poly Haven** | HDRIs, PBR materials, free models | CC0 |
| **ambientCG** | Tiling PBR textures | CC0 |
| **Quixel Megascans** | High-quality props (optional, via Epic account) | Free for non-commercial |
| **Sketchfab** | Individual CC-licensed models | CC-BY / CC0 (filter on download) |
| **Kenney.nl** | UI icons, simple sprite assets | CC0 |
| **Freesound.org** | Ambient audio, SFX | CC0 / CC-BY |
| **Google Fonts** | Cormorant Garamond (titles), Inter (body) | OFL |

### Background composition (layered diorama)

Build every scene in explicit depth layers from far to near:

1. **HDRI sky / distance** — drawn as a skybox sampling the level's HDRI. Handles sun, sky, far horizon. Also feeds IBL.
2. **Distant treeline silhouettes** — 8–12 simple low-poly trees in a wide ring, slightly desaturated, blending into the HDRI.
3. **Mid-distance trees** — 10–20 detailed 3D trees instanced at ~15–40m around the play area.
4. **Foreground vegetation** — instanced grass (10k–50k blades), flowers (1k–3k), ferns.
5. **Hero geometry** — the puzzle figure, dead center, highest detail.
6. **Particles** — pollen, fireflies, embers, etc., per-level.

Camera orbits a fixed pivot at the figure's center, so detail can be concentrated where the camera actually reaches.

### One HDRI per level
Render variants of the same forest scene in Blender at different times of day. Each `.hdr` feeds both the visible skybox and the IBL pipeline so lighting always matches the backdrop.

---

## 7. Floral Level — Detailed Reference Spec

Floral is the reference build because every other level inherits its forest pipeline.

### Asset list

| Category | Asset | Notes |
|---|---|---|
| Sky / lighting | `forest_morning.hdr` (4K equirectangular) | Custom Blender render or Poly Haven |
| Trees | 2–3 species, mid-poly, 2 LODs each | Bark + leaf split material |
| Foliage | Grass blade, 3–5 flowers, 2 ferns | Low-poly, alpha-cutout |
| Ground | Terrain mesh | Vertex-colored for material blending |
| Ground materials | Moss, dirt, leaves, rock | Tiling PBR sets |
| Props | Logs, rocks, mushrooms, branches | 6–10 pieces, scattered |
| Puzzle figure | Vined Penrose triangle, 3 separate L-pieces | Anchor vertices marked, custom Blender |
| Particles | Pollen mote, firefly | 64×64 PNGs |
| Audio (optional) | Forest ambience, trace SFX, lock SFX | Freesound CC0 |

### Render passes — the actual frame

| # | Pass | Output | Notes |
|---|------|--------|-------|
| 1 | Shadow map (sun) | Depth texture (cascaded) | Trees + figure cast; grass does not |
| 2 | G-buffer | RT0: albedo+AO, RT1: normal+roughness, RT2: emissive+metallic, depth | Alpha-tested foliage via `discard` |
| 3 | Lighting | HDR float framebuffer | Sun + shadow + IBL (diffuse irradiance + specular prefiltered) |
| 4 | Skybox | HDR framebuffer | `depth = 1.0`, `LEQUAL` |
| 5 | Volumetric god rays | Half-res, additively blended | Raymarched, samples shadow map, ~16 steps, bilateral upsample |
| 6 | Forward transparent / particles | HDR framebuffer | Pollen, fireflies, sorted back-to-front |
| 7 | Post-process chain | Backbuffer | Bloom → ACES tone map → LUT grade → vignette → FXAA |

### Foliage instancing detail

For grass: a single 3-triangle blade mesh, instanced 10k–50k times via `glDrawElementsInstanced`. Per-instance buffer contains position (vec3), rotation y-axis (float), scale (float), color tint (vec3). Wind is computed in the vertex shader as `sin(time + worldPos.x * k) * windNoise * stiffnessFromVertexColor` — trunk vertex colors mean stiff (no displacement), tip vertex colors mean floppy.

### Leaf SSS approximation

Two extra lines in the leaf fragment shader: when light is *behind* the leaf, add a translucent term based on `pow(dot(-lightDir, viewDir), p) * thicknessMap * lightColor`. Cheap, transforms the look.

---

## 8. Technical Stack & Libraries

All dependencies via CMake `FetchContent`. No external package managers. Single `cmake -B build && cmake --build build` from a fresh clone.

| Library | Purpose | Version | Citation in writeup |
|---|---|---|---|
| GLFW | Windowing + input | 3.3+ | "GLFW (Camilla Löwy et al.)" |
| glad | OpenGL function loader (4.5 Core) | latest | "glad (David Herberth)" |
| GLM | Math (vec, mat, quat) | 0.9.9+ | "OpenGL Mathematics (G-Truc Creation)" |
| Dear ImGui | Debug UI + menus | docking branch | "Dear ImGui (Omar Cornut)" |
| stb_image / stb_image_write | Image + HDR loading | latest | "stb (Sean Barrett)" |
| Assimp | Mesh loading | 5.x | "Open Asset Import Library" |
| nlohmann/json | Level files | 3.x | "JSON for Modern C++" |

**Language:** C++17. **Build system:** CMake 3.20+. **Target platforms:** Windows (MSVC) + Linux (gcc/clang). macOS is nice-to-have but Apple deprecated OpenGL.

---

## 9. Project File Structure

```
project/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── project-overview.md
├── third_party/                     # populated by FetchContent
├── assets/
│   ├── shaders/
│   │   ├── common/                  # lighting.glsl, math.glsl as #include-able files
│   │   ├── passes/                  # one .vert + .frag pair per pass
│   │   └── post/
│   ├── textures/
│   ├── models/
│   ├── hdri/
│   └── levels/
│       ├── floral.json
│       ├── stone.json
│       ├── water.json
│       ├── fire.json
│       └── cosmic.json
└── src/
    ├── core/                        # Window, Input, Time, Logger
    ├── gfx/                         # Shader, Texture, Mesh, VertexArray, Framebuffer
    ├── renderer/                    # RenderPass base, FrameContext, Renderer
    ├── scene/                       # Camera, Transform, SceneNode, Level
    ├── game/                        # PuzzleLogic, TraceController, StateMachine
    ├── ui/                          # ImGui screens (Title, ShrineSelect, Pause)
    └── main.cpp
```

A level is described entirely by its JSON file — which HDRI to load, which meshes to place where, which puzzle figure, which LUT, sun direction, particle preset. The renderer never knows about "floral" vs "stone."

---

## 10. Architecture Principles

- **Pass-based renderer.** Each rendering stage is its own class with `setup()` / `execute(FrameContext&)`. No monolithic render function.
- **RAII for GPU resources.** Shader, Texture, Mesh, Framebuffer, VertexArray own their GL handles, cleaned up in destructors. Move-only, no copies. Raw `GLuint` does not leak out.
- **Levels are data, not code.** JSON-driven. Renderer doesn't branch on level type.
- **Hot-reloadable shaders.** Watch `.glsl` mtimes; recompile on change. Set up in milestone 1.
- **No singletons** beyond what's unavoidable. The GL context is one global state machine, that's fine.

---

## 11. Milestone Roadmap

Six milestones over roughly six weeks of work. Each milestone is independently shippable — at the end of milestone N you have a working binary that demonstrates everything up to that point.

### Milestone 1 — Scaffold + minimal renderer (week 1)
CMake with FetchContent, GLFW window + GL 4.5 context, basic Shader/Texture/Mesh/Framebuffer/VAO wrappers, hot-reloadable shaders, orbit camera, one textured cube with Phong lighting, ImGui debug overlay. **Success criterion:** clone, build, run, see a cube you can orbit.

### Milestone 2 — Deferred renderer + PBR + IBL + shadows (week 2)
Deferred shading with proper G-buffer, HDR pipeline, ACES tone mapping, bloom, PBR materials, IBL from HDRI (diffuse + specular prefiltered cubemaps), skybox, cascaded shadow maps. **Success criterion:** a sphere with metallic/rough sliders, lit by an HDRI, casting shadows.

### Milestone 3 — Floral level (weeks 3–4)
Instanced foliage system, vertex wind, alpha-tested grass, leaf SSS, volumetric god rays, LUT color grading. Load the Floral scene from JSON. **Success criterion:** the Floral level renders in full visual glory (no gameplay yet).

### Milestone 4 — Puzzle logic + trace (week 5)
Alignment detection, lock animation, verlet-rope trace, geometry-shader tube rendering, level state machine, completion. **Success criterion:** Floral is fully playable end-to-end.

### Milestone 5 — Menus + polish (week 6)
Title screen, shrine select, pause menu, level transitions, settings, audio. **Success criterion:** ship-ready Floral with proper game shell around it.

### Milestone 6 — Additional levels (weeks 7+)
Port to Fire (compute particles + heat distortion), then Water (compute fluid + reflections), then Stone (tessellation + POM), then Cosmic (lensing + volumetrics) in that order. Each level reuses the engine; only its shader pack + assets + LUT differ.

---

## 12. Risk Register / Scope Cuts

If time runs short, cut from the **middle** of the difficulty curve, not the headline showcases:

- **First to cut:** Stone (mostly material work, less visually distinctive)
- **Second to cut:** Cosmic finale (most expensive single feature)
- **Never cut:** Floral (foundation), Water (signature level), Fire (visual headline)

A 3-level vertical slice (Floral + Water + Fire) still demonstrates every major pipeline stage and looks like a finished product.

---

## 13. Stretch / Expansion Ideas

If the core 5 levels finish ahead of schedule:

| Level | Adds | Effort |
|---|---|---|
| **Crystal/ice** | Cubemap refraction, chromatic aberration, translucency | ~1 week |
| **Metallic** | Full PBR metallic showcase, animated gears | ~1 week |
| **Mirror** | Stencil-based reflections, render-to-texture portals, mirror-placement puzzle | ~1.5 weeks |
| **Origami tutorial** | NPR/toon shading, edge detection — gentle intro level | ~3 days |
| **Floral variants** (autumn, night fireflies) | Free — same tech, different mood | ~1 day each |

Other expansions that aren't whole levels:
- **Shadow puzzle mechanic** — the shape isn't impossible, its *shadow* is. Move a light source to cast a Penrose silhouette.
- **Cutaway / cross-section** — drag a clipping plane through the figure to reveal disconnected geometry. Stencil-buffer showcase.
- **Walk-along character** — after lock, a small figure walks the impossible loop forever. Pure showpiece.
- **Cohesion layer** — one ambient particle system (pollen / sparks / dust / stars) running in every level with per-theme parameters. Cheap visual unifier.

---

## 14. Narrative Framing (Optional)

The player is a wanderer who finds magical shrines in a forest. Each shrine guards a fragment of something — light, sound, time, memory, gravity. The trace mechanic = completing the shrine's seal. The finale (Cosmic) is where the fragments converge. No actual writing required — just a title card per level. Course graders consistently rate projects higher when there's a sense of *product* rather than tech demo.

---

## 15. What's NOT yet specified (open questions)

These were touched on but not pinned down — fill them in before milestone 3 or 4:

- **Exact puzzle figure geometry per level** — what's the figure for Stone, Water, Fire, Cosmic in detail? Each needs to be modeled in Blender with anchor vertices marked.
- **Lock animation choreography per theme** — Floral has "vines tighten + flowers bloom." Fire = ? Water = ? Stone = ? Cosmic = ?
- **Trace VFX per theme** — vine-trace, fire-trace, water-trace, etc. Each is a fragment-shader variant on one shared geometry-shader tube.
- **Audio plan** — ambient track per level + SFX bank. Optional but high polish-per-hour.
- **Specific LUTs per level** — these are 1D color-grade textures; create in DaVinci Resolve or Photoshop and export as `.cube`/`.png`.
- **Difficulty curve / tolerance angles** — how many degrees of slack does the alignment threshold allow? Probably wider for tutorial, tighter for finale.
- **Save/progress system** — does the game remember which shrines you've completed? JSON save file is the easy answer.

---

## Appendix A — Claude Code Prompt: Milestone 1

The first concrete instruction to hand to Claude Code. After this milestone is verified working, a similar milestone-2 prompt follows.

````markdown
# Impossible Shapes OpenGL Project — Milestone 1: Scaffold + Minimal Renderer

## Project context

I'm building an interactive OpenGL game inspired by impossible-shape optical illusions (Penrose triangle, Escher figures). The core mechanic: each level shows a 3D figure built from physically disconnected geometry. The player rotates the camera to find the one viewing angle where the pieces visually align into an impossible figure. When locked, the player traces the figure's path to complete the level.

The full project will ship 5 levels, all set in a magical forest with different elemental sub-themes (Floral, Stone, Water, Fire, Cosmic). Each level showcases a different chunk of the OpenGL pipeline:
- Floral: instanced foliage, vertex-shader wind, leaf SSS, volumetric god rays
- Stone: tessellation, parallax occlusion mapping, PBR rough materials
- Water: compute-shader fluid sim, planar reflections, refraction, caustics
- Fire: compute-shader particles, screen-space heat distortion, dynamic lighting
- Cosmic: screen-space gravitational lensing, volumetric raymarching, deferred composition

This is a coursework project demonstrating understanding of the OpenGL pipeline. Both visual polish and clean, explainable architecture matter.

## Tech stack — use these exactly

- C++17
- CMake 3.20+, single `cmake -B build && cmake --build build` to compile
- GLFW 3.3+ for windowing/input
- glad loader configured for OpenGL 4.5 Core
- GLM for math
- Dear ImGui (docking branch) for UI
- stb_image / stb_image_write for image loading (handles `.hdr`)
- Assimp for model loading
- nlohmann/json for later milestones

All dependencies via CMake `FetchContent`. No vcpkg, Conan, or system packages. The repo must build from a fresh clone with no manual setup.

## Architectural principles

- Pass-based renderer: each rendering stage (shadow, G-buffer, lighting, sky, volumetric, post) is its own class with `setup()` / `execute(FrameContext&)`. No monolithic render function.
- RAII for GPU resources: Shader, Texture, Mesh, Framebuffer, VertexArray are C++ classes owning GL handles. Move-only, no copies. Raw `GLuint` does not leak out of these wrappers.
- Levels are data, not code: a level will eventually be a JSON file.
- Hot-reloadable shaders: watch `.glsl` file mtimes; recompile on change. Set this up early.
- No globals beyond what's unavoidable.

## File structure

```
project/
├── CMakeLists.txt
├── README.md
├── docs/project-overview.md           # stub for now
├── third_party/                       # populated by FetchContent
├── assets/
│   ├── shaders/{common, passes, post}/
│   ├── textures/, models/, hdri/, levels/
└── src/
    ├── core/                          # Window, Input, Time, Logger
    ├── gfx/                           # Shader, Texture, Mesh, VertexArray, Framebuffer
    ├── renderer/                      # RenderPass base, FrameContext, Renderer
    ├── scene/                         # Camera, Transform, SceneNode
    ├── game/                          # placeholder
    ├── ui/                            # placeholder
    └── main.cpp
```

## Deliverables for this milestone

1. **CMake setup** — FetchContent for GLFW, glad, GLM, ImGui, stb, Assimp. Builds on Windows (MSVC) and Linux (gcc/clang). Warnings-as-errors for our code, suppressed for third_party. Debug + Release configs.

2. **Core systems** (`src/core/`):
   - `Window`: GLFW window + OpenGL 4.5 Core context, handles resize
   - `Input`: keyboard + mouse state polled per frame, mouse delta, scroll
   - `Time`: dt, total time, frame count
   - `Logger`: LOG_INFO/WARN/ERROR macros, printf-style, file/line, ANSI color

3. **GFX layer** (`src/gfx/`):
   - `Shader`: loads vertex+fragment (and optionally compute), supports custom `#include` in GLSL via text preprocessing, hot-reloads on file change
   - `Texture`: 2D textures from file via stb_image, supports 8-bit and HDR, mipmaps, aniso
   - `Mesh`: indexed triangle mesh with position+normal+UV+tangent, has draw()
   - `VertexArray`: VAO wrapper
   - `Framebuffer`: configurable color attachments, depth/stencil, resize

4. **Scene basics** (`src/scene/`):
   - `Transform`: pos + quat rotation + scale, computes model matrix
   - `Camera`: perspective + orbit controls (drag = rotate, scroll = zoom)
   - `SceneNode`: Transform + optional Mesh*

5. **Minimal first scene**: one textured cube at origin, one directional light, basic Phong shading, orbit camera, solid color background (no skybox yet).

6. **ImGui debug overlay**: FPS, frame time, camera position, "Reload Shaders" button, sliders for light direction + color, docking enabled.

7. **Documentation**: README with build instructions + controls (LMB drag = orbit, scroll = zoom, R = reload shaders). `docs/project-overview.md` stubbed.

## Out of scope for THIS milestone — do not do yet

No PBR, IBL, shadows, deferred, volumetrics, bloom, tone mapping, post-processing. No model loading via assimp (cube hard-coded). No skybox. No puzzle logic, alignment, trace, particles. No JSON level loader. No menus or game state machine.

Don't pre-scaffold empty classes for these. Goal: a small, working, correct renderer — not a half-built engine skeleton.

## Code conventions

- PascalCase types, camelCase functions/vars, kCamelCase constants, m_ prefix for private members, snake_case filenames
- `#pragma once` headers, forward-declare where possible
- No exceptions for control flow; std::optional or return codes; assertions for programmer errors
- No singletons; Input can be a static class but no getInstance()
- Comments: explain why not what; brief doc comments on public APIs; non-obvious code only in implementations
- Includes: std lib first, then third-party, then project — alphabetical within groups

## Working agreement

- Ask before substituting any library
- Pick reasonable defaults silently for minor things; surface non-obvious decisions at the end
- Commit per logical chunk if you have git access
- Print final file tree
- Flag platform-specific concerns (file watching: use std::filesystem::last_write_time and poll, no watcher lib)
- Stop and ask if you're about to make a load-bearing architectural decision not covered here

## Success criterion

```
git clone <repo>
cd project
cmake -B build
cmake --build build
./build/impossible
```

…opens a window with a textured cube I can orbit using LMB + scroll, an ImGui debug panel shows FPS and a "Reload Shaders" button, pressing R also reloads shaders. Modifying the cube's fragment shader, hitting R, updates visuals without restarting.

Next milestone will add deferred shading, PBR, IBL from HDRI, cascaded shadow maps, skybox — the rendering core for every level.
````

---

*End of plan. Update this document as decisions are pinned down — particularly Section 15 (open questions) and the milestone-N Claude Code prompts as they're written.*