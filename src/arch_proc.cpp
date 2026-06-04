#include "obj_shape.h"
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Slot 6 — Curved Impossible Arch as POLISHED CHROME with PBR (Cook-Torrance
// + image-based environment lighting).
//
// Pipeline (executed once at init):
//   1. Build a procedural cubemap on the CPU by evaluating procSky(dir) at
//      the centre of each texel for all 6 faces.
//   2. Upload as a GL_TEXTURE_CUBE_MAP with GL_RGB16F (HDR-capable).
//   3. glGenerateMipmap → roughness mip chain. (Box-filter blur, not the
//      physically-correct GGX convolution, but plenty good for visuals.)
//
// Per-frame:
//   - Bind cubemap to texture unit 0.
//   - Draw skybox (samples the cubemap at mip 0).
//   - Draw figure with chrome shader. It samples the cubemap at
//     mip = roughness * MAX_MIP via textureLod() to get a roughness-aware
//     environment reflection. Combined with Cook-Torrance (D_GGX, G_Smith,
//     Schlick Fresnel) for direct light, plus tonemapping at the end.
//
// Post-solve animation: the skybox rotation (Y axis) accumulates while
// solved. Both the skybox draw AND the chrome's reflection sampling apply
// the same rotation, so the env appears to spin around a stationary figure.
// ─────────────────────────────────────────────────────────────────────────────

static ObjShape g_shape;

static ObjColorPalette arch_proc_palette()
{
    // For pure metal (F0 = albedo), the palette IS the metal's specular
    // tint. Dark cobalt-violet — like patinated chrome or a deep titanium
    // anodisation. Dark enough that the reflections POP off the surface.
    ObjColorPalette p;
    p.xp = vec4(0.28f, 0.22f, 0.42f, 1.0f);   // deep cobalt
    p.xn = vec4(0.18f, 0.14f, 0.30f, 1.0f);   // shadowed cobalt
    p.yp = vec4(0.34f, 0.26f, 0.48f, 1.0f);   // lit violet
    p.yn = vec4(0.22f, 0.16f, 0.34f, 1.0f);   // shadowed violet
    p.zp = vec4(0.30f, 0.24f, 0.44f, 1.0f);   // mid metal
    p.zn = vec4(0.20f, 0.16f, 0.32f, 1.0f);
    p.generic = vec4(0.26f, 0.20f, 0.38f, 1.0f);
    return p;
}

// ── Skybox geometry + program ──────────────────────────────────────────────
static GLuint sky_vao = 0, sky_vbo = 0;
static GLuint sky_program = 0;
static GLint  sky_viewLoc = -1, sky_projLoc = -1, sky_rotLoc = -1, sky_envMapLoc = -1;
static float  sky_rotation = 0.0f;
static double last_time    = 0.0;

// ── Static point light + visible marker sphere ─────────────────────────────
static GLuint mark_vao = 0, mark_vbo = 0;
static GLuint mark_program = 0;
static GLint  mark_mvpLoc = -1, mark_radLoc = -1, mark_colorLoc = -1, mark_eyeLoc = -1;
static int    mark_vertCount = 0;
// Fixed world-space key-light position — upper-right-front of the figure.
// (Was orbiting; user found the motion distracting. Static reads better.)
static const vec3  LIGHT_POS(9.0f, 10.0f, 7.0f);
static constexpr float LIGHT_MARKER_RAD = 0.85f;
// Warm-white slightly pink — distinguishable from the cool environment tones.
static vec3 g_light_color(1.0f, 0.92f, 0.85f);

// ── Procedural env cubemap (baked once on CPU) ─────────────────────────────
static GLuint env_cubemap   = 0;
static int    env_max_mip   = 0;

// Same procSky() as the shaders. Keep these in sync — if you change the
// curve here, mirror it in fshader_skybox.glsl / fshader_iridescent.glsl
// (well, the shaders sample THIS cubemap now, so they don't care — but
// future-you swapping back to inline procedural will).
static vec3 proc_sky(vec3 dir)
{
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 1e-6f) return vec3(0,0,0);
    dir.x /= len; dir.y /= len; dir.z /= len;

    float t = 0.5f * dir.y + 0.5f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Cranked-up vaporwave palette — saturated, HDR-ish so the chrome's
    // bright spots really catch fire.
    vec3 zenith (0.05f, 0.04f, 0.32f);   // near-black midnight indigo
    vec3 horizon(1.40f, 0.45f, 0.85f);   // hot magenta (>1, HDR allowed by RGB16F)
    vec3 nadir  (0.02f, 0.20f, 0.55f);   // deep ocean teal

    auto smoothstep = [](float e0, float e1, float x) {
        float u = (x - e0) / (e1 - e0);
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        return u * u * (3.0f - 2.0f * u);
    };

    vec3 col;
    if (t > 0.5f) {
        float s = smoothstep(0.5f, 1.0f, t);
        col = vec3(horizon.x + (zenith.x - horizon.x) * s,
                   horizon.y + (zenith.y - horizon.y) * s,
                   horizon.z + (zenith.z - horizon.z) * s);
    } else {
        float s = smoothstep(0.0f, 0.5f, t);
        col = vec3(nadir.x + (horizon.x - nadir.x) * s,
                   nadir.y + (horizon.y - nadir.y) * s,
                   nadir.z + (horizon.z - nadir.z) * s);
    }
    float az  = atan2f(dir.z, dir.x);
    float mod = 1.0f + 0.06f * sinf(az * 3.0f);
    col.x *= mod; col.y *= mod; col.z *= mod;
    return col;
}

// Map a face index + (u, v) in [-1, 1] to a 3D direction on the unit cube.
// Follows the Khronos/OpenGL cubemap convention.
static vec3 cube_dir(int face, float u, float v)
{
    switch (face) {
        case 0: return vec3( 1.0f, -v, -u);   // +X
        case 1: return vec3(-1.0f, -v,  u);   // -X
        case 2: return vec3( u,  1.0f,  v);   // +Y
        case 3: return vec3( u, -1.0f, -v);   // -Y
        case 4: return vec3( u, -v,  1.0f);   // +Z
        default:return vec3(-u, -v, -1.0f);   // -Z
    }
}

// Build the env cubemap on the CPU and upload it. ~50ms at size=256.
static GLuint build_proc_cubemap(int size)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    std::vector<float> face_data((size_t)size * size * 3);

    static const GLenum face_targets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                float u = ((float)x + 0.5f) / (float)size * 2.0f - 1.0f;
                float v = ((float)y + 0.5f) / (float)size * 2.0f - 1.0f;
                vec3 dir = cube_dir(face, u, v);
                vec3 col = proc_sky(dir);
                int  idx = (y * size + x) * 3;
                face_data[idx + 0] = col.x;
                face_data[idx + 1] = col.y;
                face_data[idx + 2] = col.z;
            }
        }
        glTexImage2D(face_targets[face], 0, GL_RGB16F, size, size, 0,
                     GL_RGB, GL_FLOAT, face_data.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Box-filter mip chain — used by the chrome shader to fake roughness-
    // dependent blurring of the environment reflection. (Real PBR uses
    // GGX importance-sampled convolution, but glGenerateMipmap looks
    // surprisingly close at moderate roughness values.)
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // log2(size) is the last mip level.
    int maxMip = 0;
    for (int s = size; s > 1; s >>= 1) ++maxMip;
    env_max_mip = maxMip;

    return tex;
}

// ── Skybox cube geometry ───────────────────────────────────────────────────
static void build_skybox_cube(vec3* out36)
{
    vec3 c[8] = {
        vec3(-1,-1,-1), vec3( 1,-1,-1), vec3( 1, 1,-1), vec3(-1, 1,-1),
        vec3(-1,-1, 1), vec3( 1,-1, 1), vec3( 1, 1, 1), vec3(-1, 1, 1),
    };
    int i = 0;
    auto face = [&](int a, int b, int c0, int d) {
        out36[i++] = c[a]; out36[i++] = c[b]; out36[i++] = c[c0];
        out36[i++] = c[a]; out36[i++] = c[c0]; out36[i++] = c[d];
    };
    face(0,1,2,3); face(5,4,7,6); face(4,0,3,7);
    face(1,5,6,2); face(4,5,1,0); face(3,2,6,7);
}

// UV-sphere generator — unit radius, two-triangles-per-quad. Used for the
// glowing light marker. Returns total vertex count.
static int build_uv_sphere(std::vector<vec3>& out, int stacks, int slices)
{
    out.clear();
    for (int i = 0; i < stacks; ++i) {
        float p1 = (float)M_PI * (float)i       / stacks;
        float p2 = (float)M_PI * (float)(i + 1) / stacks;
        for (int j = 0; j < slices; ++j) {
            float t1 = 2.0f * (float)M_PI * (float)j       / slices;
            float t2 = 2.0f * (float)M_PI * (float)(j + 1) / slices;
            vec3 v00(sinf(p1)*cosf(t1), cosf(p1), sinf(p1)*sinf(t1));
            vec3 v01(sinf(p1)*cosf(t2), cosf(p1), sinf(p1)*sinf(t2));
            vec3 v10(sinf(p2)*cosf(t1), cosf(p2), sinf(p2)*sinf(t1));
            vec3 v11(sinf(p2)*cosf(t2), cosf(p2), sinf(p2)*sinf(t2));
            out.push_back(v00); out.push_back(v10); out.push_back(v01);
            out.push_back(v01); out.push_back(v10); out.push_back(v11);
        }
    }
    return (int)out.size();
}

void archp_init()
{
    g_shape.init("../models/impossible_arch_curved.obj", arch_proc_palette(),
                 "../shaders/vshader_iridescent.glsl",
                 "../shaders/fshader_iridescent.glsl");

    g_shape.angleX        = 25.0f;
    g_shape.angleY        = 60.0f;
    g_shape.angleZ        = -20.0f;
    g_shape.defaultAngleX = 25.0f;
    g_shape.defaultAngleY = 60.0f;
    g_shape.defaultAngleZ = -20.0f;

    // ── Build the procedural environment cubemap once ───────────────────
    env_cubemap = build_proc_cubemap(256);
    fprintf(stderr, "slot 6: built %dx%d env cubemap, %d mip levels\n",
            256, 256, env_max_mip + 1);

    // ── Skybox program + cube VAO ───────────────────────────────────────
    sky_program  = InitShader("../shaders/vshader_skybox.glsl",
                              "../shaders/fshader_skybox.glsl");
    sky_viewLoc  = glGetUniformLocation(sky_program, "view");
    sky_projLoc  = glGetUniformLocation(sky_program, "projection");
    sky_rotLoc   = glGetUniformLocation(sky_program, "uSkyboxRotation");
    sky_envMapLoc = glGetUniformLocation(sky_program, "uEnvMap");

    glGenVertexArrays(1, &sky_vao);
    glBindVertexArray(sky_vao);
    glGenBuffers(1, &sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sky_vbo);
    vec3 cubeVerts[36];
    build_skybox_cube(cubeVerts);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    GLint posLoc = glGetAttribLocation(sky_program, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glBindVertexArray(0);

    // ── Wire the cubemap + roughness uniforms on the chrome shader ──────
    glUseProgram(g_shape.shaderProgram);
    GLint envLoc        = glGetUniformLocation(g_shape.shaderProgram, "uEnvMap");
    GLint roughLoc      = glGetUniformLocation(g_shape.shaderProgram, "uRoughness");
    GLint maxMipLoc     = glGetUniformLocation(g_shape.shaderProgram, "uMaxEnvMip");
    if (envLoc    >= 0) glUniform1i(envLoc, 0);          // sampler bound to unit 0
    if (roughLoc  >= 0) glUniform1f(roughLoc, 0.10f);    // mirror-polish chrome
    if (maxMipLoc >= 0) glUniform1f(maxMipLoc, (float)env_max_mip);

    // (Visible light marker removed — was a small glowing orb at LIGHT_POS
    // intended as a "see where your light is" indicator, but it was too
    // visually loud and read as a separate object in the middle of the
    // figure. The chrome still uses LIGHT_POS internally for its specular
    // highlight; the marker just isn't drawn anymore.)

    // The chrome shader's light position is driven by the orbit — turn on
    // ObjShape's custom-light override.
    g_shape.useCustomLight = true;
}

void archp_display()
{
    // Light is FIXED at LIGHT_POS — no orbit. The figure rotates relative
    // to the light when the user drags, which is what makes the highlights
    // move. This reads as "real physical light" much better than the
    // orbiting variant did.
    vec3 lightPos = LIGHT_POS;
    g_shape.setCustomLight(lightPos);
    g_shape.setSkyboxRotation(0.0f);   // static backdrop too

    // Cubemap stays bound to texture unit 0 — the chrome shader still uses
    // it for reflection sampling. We don't draw a skybox or override the
    // clear colour, so the background is the same pale slate as every
    // other slot (set by main.cpp's render-loop clear).
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

    g_shape.display();
}
void archp_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void archp_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void archp_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void archp_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
