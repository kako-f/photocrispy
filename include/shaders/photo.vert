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



// The Vertex Shader is the programmable Shader stage in the rendering pipeline that handles the processing of individual vertices. 
// Vertex shaders are fed Vertex Attribute data, as specified from a vertex array object by a drawing command. 
// A vertex shader receives a single vertex from the vertex stream and generates a single vertex to the output vertex stream. 
// There must be a 1:1 mapping from input vertices to output vertices. 
// https://www.khronos.org/opengl/wiki/Vertex_Shader