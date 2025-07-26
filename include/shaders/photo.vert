#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    // texture coordinates (uv),
    // from (0,0) to (1,1)
    // aAposition receive the 2D coordinate of each of our triangle’s vertices
    TexCoords = aTexCoords;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}