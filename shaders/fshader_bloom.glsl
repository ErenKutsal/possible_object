#version 150
in vec2 fragUV;
uniform sampler2D uScene;
uniform sampler2D uBloom;
out vec4 fragColor;

void main() {
    vec3 scene = texture(uScene, fragUV).rgb;
    vec3 bloom = texture(uBloom, fragUV).rgb;

    // Additive blend — bloom adds light on top
    vec3 combined = scene + bloom * 1.2;  // 1.2 = bloom intensity, tune this

    // Tone mapping: prevents values > 1 from clipping to white
    // This is the Reinhard operator — a classic CG technique
    combined = combined / (combined + vec3(1.0));

    // Gamma correction (monitor expects gamma 2.2)
    combined = pow(combined, vec3(1.0 / 2.2));

    fragColor = vec4(combined, 1.0);
}