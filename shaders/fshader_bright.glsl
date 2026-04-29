#version 150
in vec2 fragUV;
uniform sampler2D uScene;
out vec4 fragColor;

void main() {
    vec3 color = texture(uScene, fragUV).rgb;

    // Perceived brightness — human eyes weight green most
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Only pass through bright pixels — tune 0.7 to taste
    // Higher = only very bright highlights glow
    // Lower  = more of the scene gets a halo
    if (brightness > 0.7)
        fragColor = vec4(color, 1.0);
    else
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}