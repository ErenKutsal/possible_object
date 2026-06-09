#version 330 core

in vec3 fN;
in vec3 fP;

uniform vec3  cameraPos;
uniform int   u_ballStyle;  // 0 = purple glass, 5 = rainbow
uniform float u_time;       // seconds, from glfwGetTime()

out vec4 fragColor;

vec3 sampleEnv(vec3 dir)
{
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ground  = vec3(0.08, 0.07, 0.10);
    vec3 horizon = vec3(0.55, 0.55, 0.62);
    vec3 sky     = vec3(0.95, 0.97, 1.00);
    vec3 col = mix(ground, horizon, smoothstep(0.0, 0.5, t));
    col      = mix(col,    sky,     smoothstep(0.5, 1.0, t));
    return col;
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec3 N = normalize(fN);
    vec3 V = normalize(cameraPos - fP);

    vec3 L1 = normalize(vec3(-0.6,  0.8,  0.5));
    vec3 L2 = normalize(vec3( 0.5, -0.3, -0.6));

    vec3  env  = sampleEnv(reflect(-V, N));
    float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    vec3 H1 = normalize(L1 + V);
    vec3 H2 = normalize(L2 + V);

    // ---- 2 = chrome steel ball ----
    if (u_ballStyle == 2) {
        float s1 = pow(max(dot(N, H1), 0.0), 120.0);
        float s2 = pow(max(dot(N, H2), 0.0), 60.0) * 0.5;
        vec3 spec = vec3(s1 + s2) * 1.5;
        vec3 color = env;
        color += spec;
        fragColor = vec4(color, 1.0);
        return;
    }

    // ---- 5 = rainbow / iridescent ----
    if (u_ballStyle == 5) {
        float s1 = pow(max(dot(N, H1), 0.0), 90.0);
        float s2 = pow(max(dot(N, H2), 0.0), 45.0) * 0.4;
        vec3  spec = vec3(s1 + s2) * 1.3;

        float hue = fract(u_time * 0.30 + fP.y * 0.40 + dot(N, V) * 0.5);
        vec3 rainbow = hsv2rgb(vec3(hue, 0.85, 1.0));

        vec3 color = mix(rainbow, env, fres * 0.5);
        color += spec;
        color += vec3(fres * 0.3);
        fragColor = vec4(color, 1.0);
        return;
    }

    // ---- 0 = purple glass marble (default) ----
    vec3  bodyColor = vec3(0.45, 0.10, 0.55);
    vec3  refrDir   = refract(-V, N, 0.66);
    vec3  inner     = sampleEnv(refrDir) * bodyColor + bodyColor * 0.4;

    float s1 = pow(max(dot(N, H1), 0.0), 90.0);
    float s2 = pow(max(dot(N, H2), 0.0), 45.0) * 0.4;
    vec3  spec = vec3(s1 + s2) * 1.3;

    vec3 color = mix(inner, env, clamp(0.45 + fres * 0.9, 0.0, 1.0));
    color += spec;
    color += fres * 0.9 * 0.25 * vec3(0.9, 0.95, 1.0);

    fragColor = vec4(color, 1.0);
}