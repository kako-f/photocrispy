#include "opengl_rendering.h"

// Modern OpenGL requires that we at least set up a vertex and fragment shader if we want to do some rendering
// The Vertex Shader is the programmable Shader stage in the rendering pipeline that handles the processing of individual vertices.
// The fragment shader is all about calculating the color output of your pixels.
// ======
// vector "position" =  3 axis (vec.y, vec.x, vec.z) and persective division (vec.w)
// vector "color" =  RGB and Alpha

namespace OpenGlRendering
{
    void TriangleRendering::readShaderSrc(const char *fname, std::vector<char> &buffer)
    {
        // reading shader source files

        std::ifstream inFile;
        inFile.open(fname, std::ios::binary);
        if (inFile.is_open())
        {
            // Get the number of bytes stored in this file
            inFile.seekg(0, std::ios::end);
            size_t length = (size_t)inFile.tellg();
            // goto start of file
            inFile.seekg(0, std::ios::beg);
            // read the content of the file to a buffer
            buffer.resize(length + 1);
            inFile.read(&buffer[0], length);
            inFile.close();
            // ad a valid c string to the end
            buffer[length] = '\0';
        }
        else
        {
            std::cerr << "Unable to open " << fname << " I'm out!" << std::endl;
            exit(-1);
        }
    }
    unsigned int TriangleRendering::compileShader(const char *fname, GLenum shaderType)
    {
        // Load a shader from an external file
        std::vector<char> buffer;
        readShaderSrc(fname, buffer);
        const char *src = &buffer[0];

        // shader compilation -
        // shaderType dictates if vertex or fragment shader
        unsigned int newShader = glCreateShader(shaderType);
        // we attach the shader source code to the shader object and compile the shader:
        glShaderSource(newShader, 1, &src, nullptr);
        glCompileShader(newShader);
        // check result
        int test;
        glGetShaderiv(newShader, GL_COMPILE_STATUS, &test);
        if (!test)
        {
            std::cerr << "Shader compilation failed with this message:" << std::endl;
            std::vector<char> compilation_log(512);
            glGetShaderInfoLog(newShader, compilation_log.size(), NULL, &compilation_log[0]);
            std::cerr << &compilation_log[0] << std::endl;
            exit(-1);
        }
        return newShader;
    }
    unsigned int TriangleRendering::createShaderProgram(const char *vertexShaderPath, const char *fragmentShaderPath)
    {
        // The first part of the pipeline is the vertex shader that takes as input a single vertex.
        // The main purpose of the vertex shader is to transform 3D coordinates into different 3D coordinate
        // we ccreate a shader object.
        unsigned int vertexShader = compileShader(vertexShaderPath, GL_VERTEX_SHADER);

        // The main purpose of the fragment shader is to calculate the final color of a pixel and this is usually
        // the stage where all the advanced OpenGL effects occur. Usually the fragment shader contains data about
        // the 3D scene that it can use to calculate the final pixel color (like lights, shadows, color of the light and so on).
        unsigned int fragmentShader = compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

        // A shader program object is the final linked version of multiple shaders combined.
        // To use the recently compiled shaders we have to link them to a shader program object
        // and then activate this shader program when rendering objects.
        // The activated shader program's shaders will be used when we issue render calls.
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // don't forget to delete the shader objects once we've linked them into the program object;
        // we no longer need them anymore:
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    void TriangleRendering::openglInit()
    {
        // vertex array object (VAO)
        // when configuring vertex attribute pointers you only have to make those calls once and whenever we want to draw the object,
        // we can just bind the corresponding VAO. This makes switching between different vertex data and attribute configurations as easy as binding a different VAO
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

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

        shaderProgram = createShaderProgram("../../include/shaders/triangle_vertex.shader", "../../include/shaders/triangle_fragment.shader");

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
    }

    bool TriangleRendering::triangleInit(int width, int height)
    {

        glGenFramebuffers(1, &FBO);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &textureColorBuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        openglInit();

        return true;
    }
    void TriangleRendering::useProgram()
    {
        glUseProgram(shaderProgram);
    }
    void TriangleRendering::triangleRender(const float width, const float height)
    {
        // Bind FBO → render to texture
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the glProgram with the shaders
        useProgram();

        // update the uniform color
        float timeValue = glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
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
        if (FBO)
            glDeleteFramebuffers(1, &FBO);
        if (textureColorBuffer)
            glDeleteTextures(1, &textureColorBuffer);
        if (RBO)
            glDeleteRenderbuffers(1, &RBO);
        if (VAO)
            glDeleteVertexArrays(1, &VAO);
        if (VBO)
            glDeleteBuffers(1, &VBO);
        if (EBO)
            glDeleteBuffers(1, &EBO);
        if (shaderProgram)
            glDeleteProgram(shaderProgram);
    }

}
