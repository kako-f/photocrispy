#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D ourTexture; // Keep this, but don't use it for now
uniform float brightness; // Keep this, but don't use it for now

void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Output solid RED
}