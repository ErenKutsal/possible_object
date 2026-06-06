#version 150

// CHROME / METAL — Cook-Torrance BRDF with cubemap-based environment IBL.
//
// What changed from the previous version:
//   - The procedural sky is no longer computed inline. Instead, slot 6
//     bakes the procSky() function into a real cubemap texture at startup
//     and binds it as `uEnvMap` (GL_TEXTURE0). Both this shader and the
//     skybox shader sample THAT cubemap, so they're guaranteed consistent.
//   - The lighting model is now Cook-Torrance: D·F·G/(4·N·V·N·L) for the
//     direct-light specular, and a Fresnel-weighted environment sample for
//     image-based-lighting (the "ambient" specular). No more Phong.
//   - Roughness drives the mip level we sample the cubemap at — sharp at
//     roughness=0 (chrome), blurry at roughness=1 (brushed steel).
//   - GGX (Trowbridge-Reitz) normal distribution, Smith G with Schlick-
//     GGX geometry, Schlick Fresnel. Standard PBR cocktail.
//
// References:  Cook & Torrance 1982, Walter et al. 2007 (GGX),
//              Karis "Real Shading in Unreal Engine 4" (2013).

in vec3 fragPos;
in vec4 fragColor;

uniform vec3  uLightPos;
uniform vec3  uEyePos;
uniform float uTime;
uniform float uObjHeight;
uniform float uLockGlow;

uniform float uHueShift;
uniform float uIridescenceAmount;
uniform float uSkyboxRotation;       // Y-axis rotation of the env cubemap

uniform samplerCube uEnvMap;          // baked procedural sky cubemap
uniform float       uRoughness;       // 0.05 = mirror, 0.8 = brushed
uniform float       uMaxEnvMip;       // last mip level of uEnvMap

out vec4 outColor;

const float PI = 3.14159265359;

// ─── HSV → RGB (for the iridescent tint at grazing angles) ─────────────
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// ─── Cook-Torrance BRDF building blocks ────────────────────────────────
// GGX normal distribution function: probability that a microfacet has
// normal H, given the macro normal N and roughness α = roughness².
float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// ── Anisotropic GGX (Heitz 2014, Burley 2012 form) ────────────────────
// Same GGX distribution but two different α values along the surface's
// tangent (αt) and binormal (αb) axes. When αt = αb this reduces EXACTLY
// to the isotropic D_GGX above (you can verify: TdotH²+BdotH² = 1-NdotH²,
// so the bracket collapses to (1-NdotH²)/α² + NdotH² = ((α²-1)NdotH²+1)/α²
// → matches the isotropic denominator after rescaling).
//
// Geometrically: the microfacet distribution is elongated along T when
// αt > αb, so highlights stretch in that direction — the visual signature
// of brushed metal. Implementation references Karis "Real Shading in
// Unreal Engine 4" (2013) and Filament's PBR documentation.
float D_GGX_aniso(float NdotH, float TdotH, float BdotH, float at, float ab)
{
    float dt = TdotH / at;
    float db = BdotH / ab;
    float d  = dt * dt + db * db + NdotH * NdotH;
    return 1.0 / (PI * at * ab * d * d);
}

// Schlick-GGX geometry term for one direction.
float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// Smith G — combines two Schlick-GGX terms (one for view, one for light).
float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// ── Smith G for anisotropic GGX (Heitz 2014) ──────────────────────────
// Lambda function is the integrated shadowing/masking for the anisotropic
// distribution; the separable G is just G1(V)·G1(L) where G1 = 1/(1+Λ).
// Properly accounts for shadowing being stretched along the brush axis.
float Lambda_aniso(float ZdotN, float ZdotT, float ZdotB, float at, float ab)
{
    float numer = at * at * ZdotT * ZdotT + ab * ab * ZdotB * ZdotB;
    float denom = ZdotN * ZdotN;
    return (-1.0 + sqrt(1.0 + numer / max(denom, 0.0001))) * 0.5;
}
float G_Smith_aniso(
    float NdotV, float TdotV, float BdotV,
    float NdotL, float TdotL, float BdotL,
    float at, float ab)
{
    float lam_v = Lambda_aniso(NdotV, TdotV, BdotV, at, ab);
    float lam_l = Lambda_aniso(NdotL, TdotL, BdotL, at, ab);
    return 1.0 / ((1.0 + lam_v) * (1.0 + lam_l));
}

// ── Tangent frame from N alone — seam-safe brushed-metal axis ─────────
// Pick a stable "anchor" direction (world up), project it onto the face
// plane → that's the brushing axis T. B completes the right-handed frame.
// Function of N only → identical at the magic join, so the anisotropic
// highlight stays seamless across aligned bars.
void make_tangent_frame(vec3 N, out vec3 T, out vec3 B)
{
    vec3 ref = abs(N.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(ref - N * dot(ref, N));
    B = cross(N, T);
}

// Schlick approximation of Fresnel.
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Schlick-Fresnel modified for roughness — used for env-map Fresnel where
// we don't have a sharp surface direction. (Karis 2013.)
vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Rotate a direction by the skybox's Y-axis rotation.
vec3 rotateY(vec3 v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec3( c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

void main()
{
    // Flat normals via screen-space derivatives — the bars have hard
    // creases and no normal attribute, so this is the simplest correct
    // option.
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N  = normalize(cross(dx, dy));

    // View direction = CONSTANT and light = DIRECTIONAL — correct under the
    // orthographic projection this figure uses. The environment reflection
    // R = reflect(-V, N) and the direct-light highlight then depend only on the
    // face normal, never world position, so two faces that align at the magic
    // angle sample the SAME point of the env map and the chrome join stays
    // seamless. (For a curved surface N still varies smoothly, so reflections
    // keep flowing across the bars — only the position term is removed.)
    vec3 V    = normalize(uEyePos);
    vec3 L    = normalize(uLightPos);
    vec3 H    = normalize(V + L);
    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // ── Material parameters ──────────────────────────────────────────
    // For pure chrome, F0 = the base "color of the metal". The lilac
    // palette acts as a *tint* on the chrome — full metallic = 1.
    vec3  albedo    = fragColor.rgb;
    float roughness = uRoughness;
    float metallic  = 1.0;

    // Dielectric F0 = 0.04 (water/plastic). Metals override with albedo.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // ── Brushed-metal tangent frame + anisotropic roughness ──────────
    // Disney's aspect parametrisation: anisotropic ∈ [0,1] stretches the
    // GGX distribution along T while area-preserving across B, so total
    // highlight energy stays the same. anisotropic=0 → exactly isotropic;
    // 0.85 ≈ heavily brushed industrial steel. Both αt and αb are in α
    // (= roughness²) space, matching the existing D_GGX convention.
    vec3 T, B;
    make_tangent_frame(N, T, B);
    const float anisotropic = 0.85;
    float aspect = sqrt(1.0 - 0.9 * anisotropic);
    float alpha  = roughness * roughness;
    float at     = max(alpha / aspect, 0.001);
    float ab     = max(alpha * aspect, 0.001);

    // Cache the tangent-frame dot products the anisotropic D and G need.
    float TdotV = dot(T, V), BdotV = dot(B, V);
    float TdotL = dot(T, L), BdotL = dot(B, L);
    float TdotH = dot(T, H), BdotH = dot(B, H);

    // ── 1. Direct-light specular (anisotropic Cook-Torrance) ─────────
    float D = D_GGX_aniso(NdotH, TdotH, BdotH, at, ab);
    float G = G_Smith_aniso(NdotV, TdotV, BdotV, NdotL, TdotL, BdotL, at, ab);
    vec3  F = F_Schlick(VdotH, F0);

    vec3 specBRDF = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

    // Energy split: kS goes to specular, kD = (1 - kS) * (1 - metallic).
    // For metallic = 1, kD = 0 → metals have no diffuse component.
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // Strong direct light — this is the PRIMARY light source. The
    // specular highlight from this single point light tracks the orbiting
    // marker sphere, so the user can visually attribute shading to it.
    vec3 directLight = vec3(3.5);
    vec3 Lo = (kD * albedo / PI + specBRDF) * directLight * NdotL;

    // ── 2. Image-based specular (env cubemap, roughness-mipped) ──────
    // Anisotropic IBL is an open research topic; the standard fast approx
    // (McAuley 2015, Filament docs) is a "BENT NORMAL" — tilt N toward the
    // anisotropic axis so the reflection vector samples in the direction
    // the highlight should stretch. Combined with the average-roughness
    // mip level, this matches the direct-light anisotropy well enough that
    // the brushed sheen and the env reflection agree.
    vec3 anisoB    = cross(B, V);
    vec3 anisoT    = normalize(cross(anisoB, B));
    vec3 bentN     = normalize(mix(N, anisoT, anisotropic * (1.0 - roughness)));
    vec3 R         = reflect(-V, bentN);
    vec3 Rrot      = rotateY(R, uSkyboxRotation);
    float mipAlpha = sqrt(0.5 * (at + ab));            // perceptual roughness for mip
    float mip      = mipAlpha * uMaxEnvMip;
    vec3 envSpec   = textureLod(uEnvMap, Rrot, mip).rgb;

    // Fresnel-roughness modulation. IBL contribution is DIMMED (was 1.0)
    // because we want the orbiting point light to be the visually dominant
    // source — the environment just gives the chrome its colored hint, not
    // its main lighting.
    vec3 F_ibl   = F_SchlickRoughness(NdotV, F0, roughness);
    vec3 ambient = envSpec * F_ibl * 0.30;

    vec3 color = ambient + Lo;

    // ── 3. Iridescent oil-film tint (post-solve only) ────────────────
    // ADDS coloured light to the dark metal instead of multiplying through
    // it — multiplicative blending kills the iridescent peaks against a
    // dark base, so we explicitly add a Fresnel-modulated rainbow on top.
    if (uIridescenceAmount > 0.001) {
        float fresnel = pow(1.0 - NdotV, 2.0);
        float hue  = fract(fresnel * 1.5 + uHueShift);
        vec3  irid = hsv2rgb(vec3(hue, 0.85, 1.0));
        color += irid * fresnel * 0.75 * uIridescenceAmount;
    }

    // ── 4. Edge stylization (silhouette + crease) — same as before ───
    float facing     = abs(dot(N, V));
    float silhouette = smoothstep(0.30, 0.05, facing);
    float crease     = smoothstep(0.20, 0.80, length(fwidth(N)));
    float edge       = max(silhouette, crease);
    vec3  edgeColor  = vec3(0.04, 0.04, 0.07);
    color            = mix(color, edgeColor, edge);

    // ── 5. Tonemap (ACES Filmic) + gamma — cinematic contrast ────────
    // Narkowicz 2015 fit to the Academy Color Encoding System curve.
    // Way more highlight punch than Reinhard's `x/(x+1)` — bright env
    // reflections stay bright instead of getting compressed to grey.
    {
        float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
        color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    }
    color = pow(color, vec3(1.0 / 2.2));

    color += vec3(uLockGlow);
    color  = min(color, vec3(1.0));

    outColor = vec4(color, fragColor.a);
}
