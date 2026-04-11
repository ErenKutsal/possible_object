# Implementation To-Do List
## Optical Illusion Interactive Animation — OpenGL

> **Scope:** Views, transformations, projections, basic flat Phong shading.  
> **Reference:** [MarcoMoroni/CG_cw2](https://github.com/MarcoMoroni/CG_cw2)  
> **Keep it simple. Build it in order.**

---

## STEP 1 — Project setup

- [ ] Create a GLFW window (e.g. 1280×720)
- [ ] Initialize GLAD for OpenGL function loading
- [ ] Set up a basic render loop: clear color buffer → swap buffers → poll events
- [ ] Add GLM as a math library (vec3, mat4)
- [ ] Confirm a blank colored window runs without errors

---

## STEP 2 — Draw one cube on screen

- [ ] Define a hardcoded cube's vertex positions (8 vertices, 12 triangles)
- [ ] Create a VAO + VBO, upload vertex data
- [ ] Write a minimal vertex shader: `gl_Position = projection * view * model * vec4(pos, 1.0)`
- [ ] Write a minimal fragment shader: output a flat color (e.g. blue)
- [ ] Draw the cube with `glDrawElements`
- [ ] Confirm the cube appears on screen

---

## STEP 3 — Model, View, Projection matrices

- [ ] **Model matrix:** use `glm::translate` and `glm::rotate` to move/orient objects
- [ ] **View matrix:** use `glm::lookAt(eye, center, up)` — place camera at (0, 0, 5) looking at origin
- [ ] **Projection matrix:** use `glm::ortho(left, right, bottom, top, near, far)` — orthographic is required for the illusions to work
- [ ] Pass all three matrices to the vertex shader as `uniform mat4`
- [ ] Experiment: move the cube around by changing the model matrix, confirm it works
- [ ] Experiment: change the camera position in `lookAt`, confirm the view changes

---

## STEP 4 — Flat Phong shading (3 directional lights, no reflection)

> This is the key trick from Marco's project. Three lights, one per axis. Each face only picks up one light so it stays a uniform flat color — no gradients across faces.

- [ ] Add face normals to vertex data (one normal per vertex, pointing outward)
- [ ] Pass normals through the vertex shader to the fragment shader
- [ ] In the fragment shader, implement diffuse-only Phong:
  ```glsl
  float diff = max(dot(normal, lightDir), 0.0);
  color = diff * lightColor * objectColor;
  ```
- [ ] Add **3 directional lights**: one pointing down (Y), one pointing right (X), one pointing forward (Z)
- [ ] Set each face group's normal so it faces exactly one of those lights
- [ ] Add a small ambient term (e.g. `0.15 * objectColor`) so shadowed faces aren't pure black
- [ ] **Do NOT add specular/reflection** — it would reveal depth and break the illusion
- [ ] Confirm: top face is light, left face is mid, right face is dark (3-tone flat look)

---

## STEP 5 — Build the impossible objects

> Each "impossible object" is just several separate 3D meshes that look connected only from one specific camera angle. No magic needed — just careful placement.

### Level 1 — Penrose Triangle
- [ ] Build **3 L-shaped bar meshes** separately (each is just a few boxes joined together)
- [ ] Place them so that from camera angle (20, 20, -20) looking at origin they appear to form a triangle
- [ ] Each bar uses its own model matrix — they are NOT connected in 3D
- [ ] Assign the correct face normals per bar so the 3-tone shading looks right
- [ ] Test: rotate the view and confirm the seams/gaps become visible from other angles

### Level 2 — Penrose Stairs
- [ ] Build **4 staircase segment meshes**, each a simple L-shaped stair block
- [ ] Arrange them in a square loop under orthographic isometric view
- [ ] Assign 3-tone face normals (top = light, left = mid, right = dark — standard isometric convention)
- [ ] Test: confirm stairs appear to go endlessly upward from the target angle

### Level 3 — Impossible Cube
- [ ] Build a **regular cube**, then split it into two halves with different depth positions
- [ ] One vertical edge crosses in front of a horizontal edge — this is the impossible junction
- [ ] Use painter's algorithm order (draw back faces first, front faces last) to control the crossing
- [ ] Test: confirm from the front the edge crossing looks wrong-but-coherent

---

## STEP 6 — Orbiting camera (the game mechanic)

- [ ] Store camera position as **spherical coordinates**: radius, theta (horizontal), phi (vertical)
- [ ] On mouse drag: update theta and phi
- [ ] Convert spherical → Cartesian for `glm::lookAt`:
  ```cpp
  eye.x = radius * sin(phi) * cos(theta);
  eye.y = radius * cos(phi);
  eye.z = radius * sin(phi) * sin(theta);
  ```
- [ ] Pass the new view matrix to the shader every frame
- [ ] Clamp phi so the camera can't go below the floor (phi between 10° and 170°)
- [ ] Test: drag mouse and confirm the camera orbits smoothly around the object

---

## STEP 7 — Viewpoint detection (solve condition)

- [ ] Store a **target view direction** per level (the magic angle as a vec3)
- [ ] Every frame compute current view direction: `glm::normalize(origin - eye)`
- [ ] Compute the dot product: `float similarity = glm::dot(currentDir, targetDir)`
- [ ] When `similarity > 0.998` (about 3–4° tolerance) → illusion is solved
- [ ] Show a simple debug readout on the window title: `glfwSetWindowTitle(window, "angle: X°")` — useful during development

---

## STEP 8 — Level select UI screen

> Simple fullscreen menu using 2D quads drawn in screen space.

- [ ] Create a separate orthographic projection for 2D UI: `glm::ortho(0, width, 0, height)`
- [ ] Draw **3 colored rectangles** representing the 3 levels (one per level)
- [ ] Label each with the level name — either use a simple texture with text baked in, or a bitmap font library like `stb_truetype`
- [ ] Detect mouse click on each rectangle using cursor position vs rectangle bounds
- [ ] On click: set `currentLevel = N` and trigger the zoom-in transition
- [ ] Draw a small preview of each illusion object (can be a static thumbnail texture, or a mini live render)

---

## STEP 9 — Zoom-in transition (menu → gameplay)

- [ ] When a level is clicked, animate the camera **zooming into** that level's card
- [ ] Interpolate: `radius = glm::mix(startRadius, gameRadius, t)` where t goes 0 → 1 over ~0.8 seconds
- [ ] Use `glfwGetTime()` to drive t: `t = (currentTime - startTime) / duration`
- [ ] While t < 1.0, disable mouse orbit (camera is animating)
- [ ] When t >= 1.0: switch to gameplay mode, enable orbit
- [ ] Add a zoom-out on ESC to return to the menu

---

## STEP 10 — Solve animation

> Keep it simple: a glowing sphere rolling along the object. Uses the same transforms you already know.

- [ ] On solve: play a **ring flash** — draw a billboard quad (flat plane always facing camera) at the object center, scale it from 0 to 2 over 0.4 seconds, fade alpha out
- [ ] Spawn a **small sphere mesh** (can be a low-poly icosphere, ~20 triangles)
- [ ] Animate it along a parametric path on the object:
  - Level 1 (Penrose triangle): move along 3 line segments in a loop, `t` goes 0 → 1 → repeats
  - Level 2 (Penrose stairs): move along 4 stair segments in a loop
  - Level 3 (Impossible cube): orbit one face with `sin/cos` on the face plane
- [ ] Color the sphere gold `(1.0, 0.7, 0.0)` with the same flat Phong shader, higher ambient
- [ ] Brighten the scene slightly: multiply the ambient term by 1.4 for 0.5 seconds on solve
- [ ] After animation starts, allow the player to freely orbit again (the animation keeps playing)

---

## STEP 11 — Level progression

- [ ] Track which levels are solved with a `bool solved[3]` array
- [ ] On the menu screen, show solved levels with a checkmark or different color
- [ ] On returning to menu after a solve, the zoom-out plays in reverse

---

## Recommended file structure

```
src/
  main.cpp          ← window, render loop, input
  camera.cpp/.h     ← spherical orbit, lookAt, viewpoint detection
  mesh.cpp/.h       ← VAO/VBO creation, draw call
  shader.cpp/.h     ← compile/link shaders, set uniforms
  level.cpp/.h      ← level data, object positions, target angles
  ui.cpp/.h         ← menu quads, click detection, zoom transition
  animation.cpp/.h  ← solve flash, moving sphere path
res/
  shaders/
    object.vert / object.frag   ← 3D Phong shader
    ui.vert / ui.frag           ← flat 2D color shader
    flash.vert / flash.frag     ← billboard alpha fade
```

---

## What you are practicing at each step

| Steps | Concept |
|---|---|
| 2–3 | Model / View / Projection matrices, `glm::lookAt`, `glm::ortho` |
| 4 | Vertex normals, fragment shader, directional lighting (Phong diffuse) |
| 5 | Mesh construction, how impossible objects work via separate transforms |
| 6 | Spherical coordinates → Cartesian, camera orbit |
| 7 | Dot product for angle comparison (view-space math) |
| 8–9 | 2D orthographic UI projection, lerp animation, time-based transitions |
| 10 | Parametric path animation, billboard technique |

---

## Things to borrow from Marco's repo

- `CMakeLists.txt` — the dependency setup (GLFW, GLAD, GLM) is already configured, use it as a base
- The orthographic projection call: `glm::ortho(-screen_width, screen_width, -screen_height, screen_height, near, far)`
- The free camera toggle (key `2`) is useful as a debug mode — keep it in your project
- The FBO post-process chain structure — you can strip it down to just the solve flash

---

## Keep in mind

- The illusion **only works with orthographic projection** (`glm::ortho`). If you switch to perspective (`glm::perspective`) the depth is visible and the seams show. That's fine — toggling between the two is a nice debug feature (Marco used key `O` for this).
- All three lights should be **directional** (just a direction vector, no position). No attenuation needed.
- The solve threshold `0.998` is a starting point — adjust it during testing. Too tight = frustrating. Too loose = trivial.
- Don't use `glBegin/glEnd` (legacy). Stick to VAO/VBO for everything.