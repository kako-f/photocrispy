#include "opengl_rendering.h"

// Modern OpenGL requires that we at least set up a vertex and fragment shader if we want to do some rendering
// The Vertex Shader is the programmable Shader stage in the rendering pipeline that handles the processing of individual vertices.
// The fragment shader is all about calculating the color output of your pixels.
// ======
// vector "position" =  3 axis (vec.y, vec.x, vec.z) and persective division (vec.w)
// vector "color" =  RGB and Alpha

namespace OpenGlRendering
{
    void TriangleRendering::openglInit()
    {
        // build and compile our shader program
        textureShader = new PhotoShader("../../include/shaders/triangle_vertex.vert", "../../include/shaders/triangle_fragment.frag");
        // === Triangle ===
        // range between -1.0 and 1.0 on all 3 axes (x, y and z)

        float vertices[] = {
            // y,x,z
            // first triangle
            0.0f, 0.0f, 0.0f,   // left
            0.5f, 0.0f, 0.0f,   // right
            0.25f, 0.5f, 0.0f,  // top
                                // first triangle
            0.0f, 0.0f, 0.0f,   // left
            -0.5f, 0.0f, 0.0f,  // right
            -0.25f, 0.5f, 0.0f, // top
                                // sec triangle
            0.0f, 0.0f, 0.0f,   // left
            -0.5f, 0.0f, 0.0f,  // right
            -0.25f, 0.5f, 0.0f, // top
                                // third triangle
            -0.25f, 0.5f, 0.0f, // left
            0.25f, 0.5f, 0.0f,  // right
            0.0f, 1.0f, 0.0f,   // top
        };

        // vertex array object (VAO)
        // when configuring vertex attribute pointers you only have to make those calls once and whenever we want to draw the object,
        // we can just bind the corresponding VAO. This makes switching between different vertex data and attribute configurations as easy as binding a different VAO
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        // Create a Vector Buffer Object (VBO) that will store the vertices on video memory
        // VBO can store a large number of vertices in the GPU's memory. They have unique ID
        // OpenGL allows us to bind to several buffers at once as long as they have a different buffer type.
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // glBufferData function  copies the previously defined vertex data into the buffer's memory
        // Arguments
        // type of buffer
        // size in bytes of data to be copied
        // actual data to sent
        // The fourth parameter specifies how we want the graphics card to manage the given data. This can take 3 forms:
        // GL_STREAM_DRAW: the data is set only once and used by the GPU at most a few times.
        // GL_STATIC_DRAW: the data is set only once and used many times.
        // GL_DYNAMIC_DRAW: the data is changed a lot and used many times.
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // shaderProgram = createShaderProgram("../../include/shaders/triangle_vertex.shader", "../../include/shaders/triangle_fragment.shader");

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void TriangleRendering::rescaleFBO(float width, float height)
    {
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
    }

    bool TriangleRendering::triangleInit(float width, float height)
    {

        glGenFramebuffers(1, &FBO);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &textureColorBuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            fmt::print("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        openglInit();

        return true;
    }

    void TriangleRendering::triangleRender(const float width, const float height)
    {
        // Bind FBO → render to texture
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        rescaleFBO(width, height);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        // make sure we clear the framebuffer's content
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the glProgram with the shaders
        textureShader->use();

        // update the uniform color
        float timeValue = (float)glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        int vertexColorLocation = glGetUniformLocation(textureShader->ID, "ourColor");
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

        glBindVertexArray(VAO);
        // GL_Triangles = opengl primitive.
        // Some of these hints are GL_POINTS, GL_TRIANGLES and GL_LINE_STRIP
        glDrawArrays(GL_TRIANGLES, 0, 12);
        // glDrawElements to indicate we want to render the triangles from an index buffer.
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to screen
        
    }

    void TriangleRendering::triangleCleanup()
    {
        glDeleteFramebuffers(1, &FBO);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &RBO);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(textureShader->ID);
    }

}
