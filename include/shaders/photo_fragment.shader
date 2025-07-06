#version 330 core
out vec4 FragColor; // The final color of the pixel

// We still receive TexCoord, but we won't use it yet for a solid color
in vec2 TexCoord; 

void main()
{
    // Output a solid red color (R=1.0, G=0.0, B=0.0, Alpha=1.0)
    FragColor = vec4(1.0, 1.0, 0.0, 1.0); 
}