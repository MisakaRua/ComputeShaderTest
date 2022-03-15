#include <cassert>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>

#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLFWwindow* window = nullptr;
static constexpr uint32_t kWndWidth = 1280;
static constexpr uint32_t kWndHeight = 720;

static constexpr size_t kParticleGroupSize = 1024;
static constexpr size_t kParticelGroupCount = 8192;
static constexpr size_t kParticleCount = kParticelGroupCount * kParticleGroupSize;

static constexpr size_t kMaxAttractors = 64;

int main(int argc, char** argv)
{
    assert(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(kWndWidth, kWndHeight, "Compute Shader", nullptr, nullptr);
    assert(window);
    glfwMakeContextCurrent(window);

    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

    glViewport(0, 0, kWndWidth, kWndHeight);


    uint32_t program = 0;
    {
        std::ifstream shaderFile("shader/particle.comp");
        assert(shaderFile.is_open());

        std::stringstream ss;
        ss << shaderFile.rdbuf();
        shaderFile.close();

        std::string shaderStr = ss.str();
        const char* shaderSource = shaderStr.c_str();


        int success = GL_TRUE;
        char infoLog[512];


        uint32_t cs = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(cs, 1, &shaderSource, nullptr);
        glCompileShader(cs);
        glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(cs, 512, NULL, infoLog);
            std::cerr << "Compute shader compiling failed.\n" << infoLog << std::endl;
            assert(false);
        };


        program = glCreateProgram();
        glAttachShader(program, cs);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cerr << "Compute shader linking failed.\n" << infoLog << std::endl;
            assert(false);
        }

        glDeleteShader(cs);
    }
    uint32_t dtLocation = glGetUniformLocation(program, "dt");


    uint32_t render = 0;
    {
        int success = GL_TRUE;
        char infoLog[512];

        static const char* render_vs =
            "#version 430 core                                  \n"
            "                                                   \n"
            "in vec4 vert;                                      \n"
            "                                                   \n"
            "uniform mat4 mvp;                                  \n"
            "                                                   \n"
            "out float intensity;                               \n"
            "                                                   \n"
            "void main(void)                                    \n"
            "{                                                  \n"
            "    intensity = vert.w;                            \n"
            "    gl_Position = mvp * vec4(vert.xyz, 1.0);       \n"
            "}                                                  \n";
        uint32_t vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &render_vs, nullptr);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            std::cerr << "Vertex shader compiling failed.\n" << infoLog << std::endl;
            assert(false);
        };

        static const char* render_fs =
            "#version 430 core                                  \n"
            "                                                   \n"
            "layout (location = 0) out vec4 color;              \n"
            "                                                   \n"
            "in float intensity;                                \n"
            "                                                   \n"
            "void main(void)                                    \n"
            "{                                                  \n"
            "    color = mix(vec4(0.0f, 0.2f, 1.0f, 1.0f), vec4(0.2f, 0.05f, 0.0f, 1.0f), intensity);   \n"
            "}                                                  \n";
        uint32_t fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &render_fs, nullptr);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            std::cerr << "Fragment shader compiling failed.\n" << infoLog << std::endl;
            assert(false);
        };


        render = glCreateProgram();
        glAttachShader(render, vs);
        glAttachShader(render, fs);
        glLinkProgram(render);
        glGetProgramiv(render, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(render, 512, NULL, infoLog);
            std::cerr << "Render shader linking failed.\n" << infoLog << std::endl;
            assert(false);
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }


    uint32_t vao = 0;
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }


    uint32_t position_buffer = 0; // the buffer to store the data
    uint32_t position_texture_buffer = 0; // the texture to let shader know what is in the buffer
    {
        glGenBuffers(1, &position_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * kParticleCount, nullptr, GL_DYNAMIC_COPY);

        std::default_random_engine e;
        std::uniform_real_distribution floatGen(0.0f, 1.0f);

        auto positions = (glm::vec4*)glMapBufferRange(GL_ARRAY_BUFFER, 0, kParticleCount * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        for (size_t i = 0; i < kParticleCount; ++i)
        {
            glm::vec3 p = glm::normalize(glm::vec3(floatGen(e), floatGen(e), floatGen(e)));
            p = 20.0f * p - 10.0f;
            positions[i] = { p.x, p.y, p.z, floatGen(e) };
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);


        glGenTextures(1, &position_texture_buffer);
        glBindTexture(GL_TEXTURE_BUFFER, position_texture_buffer);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, position_texture_buffer);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);


    uint32_t velocity_buffer = 0;
    uint32_t velocity_texture_buffer = 0;
    {
        glGenBuffers(1, &velocity_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, velocity_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * kParticleCount, nullptr, GL_DYNAMIC_COPY);

        std::default_random_engine e;
        std::uniform_real_distribution vec3Gen(-10.0f, 10.0f);

        auto velocities = (glm::vec4*)glMapNamedBufferRange(velocity_buffer, 0, kParticleCount * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        for (size_t i = 0; i < kParticleCount; ++i)
        {
            velocities[i] = { vec3Gen(e), vec3Gen(e), vec3Gen(e), 0.0f };
        }
        glUnmapBuffer(GL_ARRAY_BUFFER); //glUnmapNamedBuffer(buf); is also useful;


        glGenTextures(1, &velocity_texture_buffer);
        glBindTexture(GL_TEXTURE_BUFFER, velocity_texture_buffer);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, velocity_texture_buffer);
    }


    uint32_t attractor_buffer = 0;
    {
        glGenBuffers(1, &attractor_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, attractor_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, attractor_buffer); // the attractor buffer will be bound to binding 0.
    }
    std::vector<float> attractorMasses(kMaxAttractors);
    {
        std::default_random_engine e;
        std::uniform_real_distribution r(50000.0f, 100000.0f);

        for (size_t i = 0; i < kMaxAttractors; ++i)
        {
            attractorMasses[i] = r(e);
        }
    }


    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        double newTime = glfwGetTime();
        double dt = (newTime - lastTime);
        if (dt < 0.01) continue;


        auto attractors = (glm::vec4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 32 * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        for (int i = 0; i < 32; ++i)
        {
            attractors[i] = glm::vec4(
                glm::sin(newTime * (float)(i + 4) * 7.5f * 20.0f) * 50.0f,
                glm::cos(newTime * (float)(i + 7) * 3.9f * 20.0f) * 50.0f,
                glm::sin(newTime * (float)(i + 3) * 5.3f * 20.0f) * glm::cos(newTime * (float)(i + 5) * 9.1f) * 100.0f,
                attractorMasses[i]);
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);

        if (dt >= 2.0) dt = 2.0;


        {
            glUseProgram(program);
            glBindImageTexture(0, velocity_texture_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindImageTexture(1, position_texture_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

            glUniform1f(dtLocation, dt);

            glDispatchCompute(kParticelGroupCount, 1, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        {
            glm::mat4 mvp = glm::perspective(45.0f, (float)kWndWidth / kWndHeight, 0.1f, 1000.0f)
                * glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, -160.0f })
                * glm::rotate(glm::mat4(1.0f), (float)glm::radians(15.0f * dt), { 0.0f, 1.0f, 0.0f });

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glUseProgram(render);
            glUniformMatrix4fv(glGetUniformLocation(render, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(vao);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);

            glDrawArrays(GL_POINTS, 0, kParticleCount);

            lastTime = newTime;
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwDestroyWindow(window);
    glfwTerminate();
}