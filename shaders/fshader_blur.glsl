#version 150
in vec2 fragUV;
uniform sampler2D uScene;
uniform int uHorizontal;
out vec4 fragColor;

// Standard 9-tap Gaussian weights
float weights[5] = float[](0.227027, 0.194595, 0.121622,
                            0.054054, 0.016216);

void main() {
    vec2 texel = 1.0 / vec2(textureSize(uScene, 0));
    vec3 result = texture(uScene, fragUV).rgb * weights[0];

    if (uHorizontal == 1) {
        for (int i = 1; i < 5; i++) {
            result += texture(uScene, fragUV + vec2(texel.x * float(i), 0.0)).rgb
                      * weights[i];
            result += texture(uScene, fragUV - vec2(texel.x * float(i), 0.0)).rgb
                      * weights[i];
        }
    } else {
        for (int i = 1; i < 5; i++) {
            result += texture(uScene, fragUV + vec2(0.0, texel.y * float(i))).rgb
                      * weights[i];
            result += texture(uScene, fragUV - vec2(0.0, texel.y * float(i))).rgb
                      * weights[i];
        }
    }
    fragColor = vec4(result, 1.0);
}