# Models

8 impossible-figure 3D models for use in the `possible_object` project.

## Files

### `*.obj` — wavefront mesh files (load these in your renderer)

| File | Source | What it is |
|---|---|---|
| `penrose_triangle.obj` | Paradox Toolkit primitive | Classic Penrose triangle |
| `reutersvard_rectangle.obj` | Paradox Toolkit primitive | Reutersvärd impossible rectangle |
| `impossible_arch.obj` | Paradox Toolkit primitive | Impossible arch |
| `impossible_cube.obj` | Paradox Toolkit primitive | Impossible cube |
| `penrose_stair.obj` | Paradox Toolkit primitive | Penrose stairs |
| `mescher_window.obj` | meschers (PyTorch lib) | Impossible window |
| `g029_triple_triangle.obj` | 3× Penrose triangle composition | Decorative |
| `g100_diamond.obj` | 2× Penrose triangle composition | Decorative |

### `mescher_window.cam.json` — camera spec for the mescher window
The mescher window only looks impossible from one specific orthographic camera position. This JSON has the exact eye/target/up/frustum values used by the meschers renderer. Use it when loading `mescher_window.obj`.

### `renders/` — reference PNGs (workbench renders from Blender)
What each figure *should* look like when correctly rendered with its axonometric camera.

### `blender_source/` — `.blend` files (per-figure, plus `_all_in_one.blend`)
Open in Blender to inspect/modify each figure. Each has the **Paradox Toolkit axonometric camera** already configured (the magic angle for the illusion). `_all_in_one.blend` has all 8 figures in separate collections.

## Important: rendering caveats

- **Use orthographic projection.** Every one of these figures relies on parallel projection — perspective will visibly break the illusion.
- **Don't weld vertices on import.** Particularly for `mescher_window.obj` — the file's header comment explicitly warns: "shared 2D positions intentionally disagree in z between adjacent faces." If you merge them you destroy the illusion.
- **Look from the right angle.** Each `.blend` file has its axonometric camera pre-set. For the Paradox figures it's at `(14.43, -14.43, 14.43)` looking at origin. For `mescher_window` use the values in `mescher_window.cam.json`.
- **Use a rasterization engine, not ray tracing.** Ray tracing leaks depth information that breaks the illusion (per Matías Garate's talk).
- **Disable sun shadows** if you add directional lights — a single sun would cast a shadow across the illusion's "fake" connection point and reveal it. Use HDRI lighting + custom-distance local lights instead.

## Sources

- Paradox Toolkit Blender addon: https://github.com/matgarate/Blender_ParadoxToolkit
- meschers: PyTorch lib for optimizing impossible figures (the figures are generated via gradient descent on mesh vertex Z values to satisfy depth-ordering constraints)
- Talk reference: Matías Garate, "Modeling Impossible Figures in Blender" (Blender Conference)
