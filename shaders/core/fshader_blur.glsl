#version 150

in  vec2 fragUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform int       uHorizontal;

// 9-tap Gaussian kernel weights (sum ≈ 1.0)
const float W[5] = float[](0.22703, 0.19459, 0.12163, 0.05399, 0.01621);

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(uScene, 0));
    vec2 step  = (uHorizontal == 1) ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    vec3 result = texture(uScene, fragUV).rgb * W[0];
    for (int i = 1; i < 5; i++) {
        result += texture(uScene, fragUV + step * float(i)).rgb * W[i];
        result += texture(uScene, fragUV - step * float(i)).rgb * W[i];
    }
    FragColor = vec4(result, 1.0);
}
