#version 150

in  vec2 fragUV;
out vec4 FragColor;

uniform sampler2D uScene;

void main()
{
    vec3  col        = texture(uScene, fragUV).rgb;
    float brightness = dot(col, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.82)
        FragColor = vec4(col, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
