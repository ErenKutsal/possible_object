#version 150

in  vec2 fragUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;

void main()
{
    vec3 scene = texture(uScene, fragUV).rgb;
    vec3 bloom = texture(uBloom, fragUV).rgb;

    // Additive combine — bloom weight tuned so it's visible but not blinding.
    vec3 hdr = scene + bloom * 0.75;

    // Reinhard tone-map into [0,1] then gamma-correct.
    vec3 mapped = hdr / (hdr + vec3(1.0));
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
