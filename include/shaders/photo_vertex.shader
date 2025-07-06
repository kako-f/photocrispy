#version 330 core
layout (location = 0) in vec3 aPos;     // Vertex position (x, y, z)
layout (location = 1) in vec2 aTexCoord; // Texture coordinates (u, v)

out vec2 TexCoord; // Pass texture coordinates to the fragment shader

void main()
{
    // gl_Position determines the final position of the vertex on the screen.
    // Our quad vertices are already in Normalized Device Coordinates (-1.0 to 1.0).
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    TexCoord = aTexCoord; // Simply pass the texture coordinates through
}