# Impossible Objects

Interactive OpenGL viewer for impossible figures. Seven shapes, navigable from a
title screen and a "shrine select" menu.

## Shapes

| # | Name | Source |
|---|------|--------|
| 1 | Impossible Polygon | Parametric n-gon, hand-coded (`impossible_polygon.cpp`) |
| 2 | Penrose Triangle | OBJ: `models/penrose_triangle.obj` (Paradox Toolkit) |
| 3 | Impossible Cube | OBJ: `models/impossible_cube.obj` (Paradox Toolkit) |
| 4 | Penrose Stair | OBJ: `models/penrose_stair.obj` (Paradox Toolkit) |
| 5 | Impossible Arch | OBJ: `models/impossible_arch.obj` (Paradox Toolkit) |
| 6 | Reutersvärd Rectangle | OBJ: `models/reutersvard_rectangle.obj` (Paradox Toolkit) |
| 7 | Mescher Window | OBJ: `models/mescher_window.obj` (meschers PyTorch lib) |

Shapes 2–7 are loaded from OBJ files at runtime via `src/obj_loader.h`.
Shape 1 remains hand-coded because no OBJ equivalent exists (it has a
user-adjustable segment count and a moving ball that wraps the impossible
seam — that animation logic doesn't transfer to a static mesh).

Each OBJ-backed shape's code is a thin wrapper around the shared `ObjShape`
helper in `src/obj_shape.h`. Adding a new figure is ~10 lines of code:
copy `src/reutersvard.cpp`, change the OBJ path, register in `main.cpp`.

## Setup & Build

Requirements:
- CMake 3.14+
- GLFW 3
- OpenGL
- GLEW (only on non-macOS)

ImGui is fetched automatically via CMake `FetchContent`.

```bash
mkdir -p build && cd build
cmake ..
make
./imposible.out
```

The executable expects to be run from the `build/` directory (shader and OBJ
paths are written as `../shaders/…` and `../models/…`).

## Controls

- **Title screen** → click **Begin Journey** or **Shrine Select**
- **In a shape**:
  - Drag mouse = rotate
  - Scroll = tilt around the view axis
  - Arrow keys = step rotate
  - `R` = reset to the axonometric magic angle (the angle where the illusion works)
  - `Tab` = next shape, `1`–`7` = jump directly
  - `Esc` = back to shrine select

## Why orthographic?

Every impossible figure relies on parallel projection — perspective
foreshortening reveals the depth offsets and breaks the illusion. All
shapes render with an orthographic camera. The `ObjShape` helper bakes the
canonical axonometric viewpoint (X = 54.736°, Z = −45°) as the default rotation.

## Project structure

```
src/
├── main.cpp                  ← window, callback dispatch, shape registry
├── ui.cpp / ui.h             ← ImGui title / shrine select screens
├── includes.h                ← math types (vec3/vec4/mat4), shader loader
├── obj_loader.h              ← tiny Wavefront OBJ parser (no external deps)
├── obj_shape.h               ← shared OpenGL state + handlers for OBJ-loaded shapes
│
├── impossible_polygon.cpp    ← #1, parametric (with bouncing ball)
├── penrose.cpp               ← #2, wraps ObjShape + penrose_triangle.obj
├── neckercube.cpp            ← #3, wraps ObjShape + impossible_cube.obj
├── penrose_blocks.cpp        ← #4, wraps ObjShape + penrose_stair.obj
├── arch.cpp                  ← #5, wraps ObjShape + impossible_arch.obj
├── reutersvard.cpp           ← #6, wraps ObjShape + reutersvard_rectangle.obj
└── mescher_window.cpp        ← #7, wraps ObjShape + mescher_window.obj
shaders/
├── vshader_impossible.glsl   ← MVP transform + pass-through colour
└── fshader_impossible.glsl   ← Phong + Escher gradient + silhouette/crease edges
models/
├── *.obj                     ← 8 figure meshes (see models/README.md)
├── *.cam.json                ← camera spec for mescher_window
├── renders/                  ← reference PNGs
└── blender_source/           ← .blend files for editing in Blender
```

## Sources & credits

- Hand-coded shapes: original work
- OBJ primitives (shapes 2–6): exported from the
  [Paradox Toolkit](https://github.com/matgarate/Blender_ParadoxToolkit) Blender addon by Matías Garate
- Mescher Window (shape 7): output of the **meschers** PyTorch optimizer
- ImGui by Omar Cornut, fetched via CMake
- The fragment shader's edge-detection technique is inspired by Inglis (Bridges 2014)
