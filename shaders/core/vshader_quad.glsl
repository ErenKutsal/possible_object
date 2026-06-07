#version 150

// Fullscreen quad: two attributes bound at locations 0 and 1
// via glBindAttribLocation before link.
in vec2 aPos;  // NDC [-1, 1]
in vec2 aUV;   // [0, 1]

out vec2 fragUV;

void main()
{
    fragUV      = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
