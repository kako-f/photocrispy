#version 330 core

in vec2 position;
out vec2 texCoords;

void main() {
    texCoords = (position + 1.0) / 2.0; // map from [-1, 1] to [0, 1]
    gl_Position = vec4(position, 0.0, 1.0);
}

// The Vertex Shader is the programmable Shader stage in the rendering pipeline that handles the processing of individual vertices. 
// Vertex shaders are fed Vertex Attribute data, as specified from a vertex array object by a drawing command. 
// A vertex shader receives a single vertex from the vertex stream and generates a single vertex to the output vertex stream. 
// There must be a 1:1 mapping from input vertices to output vertices. 
// https://www.khronos.org/opengl/wiki/Vertex_Shader