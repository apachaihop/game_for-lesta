#include <SDL.h>
#include <SDL_init.h>
#include <SDL_video.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "glad/glad.h"
#include "stb_image.h"

#include "shader.hxx"
#include "texture.hxx"

#include "engine.hxx"
namespace eng
{
#define OM_GL_CHECK()                                                          \
    {                                                                          \
        const int err = static_cast<int>(glGetError());                        \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            switch (err)                                                       \
            {                                                                  \
                case GL_INVALID_ENUM:                                          \
                    std::cerr << "GL_INVALID_ENUM" << std::endl;               \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    std::cerr << "GL_INVALID_VALUE" << std::endl;              \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    std::cerr << "GL_INVALID_OPERATION" << std::endl;          \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"            \
                              << std::endl;                                    \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    std::cerr << "GL_OUT_OF_MEMORY" << std::endl;              \
                    break;                                                     \
            }                                                                  \
            assert(false);                                                     \
        }                                                                      \
    }

static void APIENTRY
callback_opengl_debug(GLenum                       source,
                      GLenum                       type,
                      GLuint                       id,
                      GLenum                       severity,
                      GLsizei                      length,
                      const GLchar*                message,
                      [[maybe_unused]] const void* userParam);

bool already_exist = false;
class CKeys
{
    SDL_KeyCode key;
    std::string name;
    event       ev;

public:
    CKeys() {}
    CKeys(SDL_KeyCode k, std::string n, enum event e)
        : key(k)
        , name(n)
        , ev(e)
    {
    }
    SDL_KeyCode get_code() const { return this->key; }
    std::string get_name() { return this->name; }
    enum event  get_event() { return ev; }
};

class engine_impl final : public eng::engine
{
    SDL_Window*        window  = nullptr;
    SDL_GLContext      context = nullptr;
    std::string        flag;
    std::vector<CKeys> binded_keys;
    unsigned int       ID;

public:
    bool initialize_engine() final;

    void draw_triangle(eng::triangle t1, eng::triangle t2) final;

    int load_texture(std::string path) final;

    bool draw_texture(eng::triangle t1,
                      eng::triangle t2,
                      int           texHandle,
                      glm::mat4     transform) final;
    bool get_input(event& e) final;
    bool rebind_key() final;
    bool swap_buff() final
    {
        SDL_GL_SwapWindow(window);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        OM_GL_CHECK()
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        OM_GL_CHECK()
        return true;
    }
};
bool engine_impl::rebind_key()
{
    std::cout << "Choose key to rebind" << std::endl;
    std::string key_name;
    std::cin >> key_name;
    auto it = std::find_if(binded_keys.begin(),
                           binded_keys.end(),
                           [&](CKeys& k) { return k.get_name() == key_name; });
    if (it == binded_keys.end())
    {
        std::cout << "No such name" << std::endl;
    }
    SDL_KeyCode kc = (SDL_KeyCode)SDL_GetKeyFromName(it->get_name().c_str());
    CKeys       new_key{ kc, key_name, it->get_event() };
    binded_keys.erase(it);
    binded_keys.push_back(new_key);
}
bool engine_impl::initialize_engine()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Error", "Cannot init SDL .", NULL);

        return false;
    }

    atexit(SDL_Quit);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window =
        SDL_CreateWindow("GLES3.2", eng::width, eng::height, SDL_WINDOW_OPENGL);
    if (!window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Error",
                                 "Couldn't create the main window.",
                                 NULL);
        return false;
    }
    binded_keys = { { SDLK_w, "up", event::up },
                    { SDLK_a, "left", event::left },
                    { SDLK_s, "down", event::down },
                    { SDLK_d, "right", event::right },
                    { SDLK_LCTRL, "button_one", event::button_one },
                    { SDLK_SPACE, "button_two", event::button_two },
                    { SDLK_ESCAPE, "select", event::select },
                    { SDLK_RETURN, "start", event::start } };

    context = SDL_GL_CreateContext(window);
    if (!context)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Error",
                                 "Couldn't create an OpenGL context.",
                                 NULL);
        return false;
    }

    int result;
    int gl_major_version;
    result =
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_major_version);

    if (gl_major_version < 3)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Error", "gl version lower then 3.", NULL);
        return false;
    }

    auto load_gl_pointer = [](const char* function_name)
    {
        SDL_FunctionPointer function_ptr = SDL_GL_GetProcAddress(function_name);
        return reinterpret_cast<void*>(function_ptr);
    };

    if (gladLoadGLES2Loader(load_gl_pointer) == 0)
    {
        std::clog << "error: failed to initialize glad" << std::endl;
    }

    glEnable(GL_DEBUG_OUTPUT);
    OM_GL_CHECK()
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    OM_GL_CHECK()
    glDebugMessageCallback(callback_opengl_debug, nullptr);
    OM_GL_CHECK()
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    OM_GL_CHECK()
    glEnable(GL_BLEND);
    OM_GL_CHECK();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OM_GL_CHECK();
    glViewport(0, 0, width, height);
    return true;
}
int engine_impl::load_texture(std::string path)
{
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data =
        stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    OM_GL_CHECK()
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    OM_GL_CHECK()

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stbi_image_free(data);
    return texture;
}

void engine_impl::draw_triangle(eng::triangle t1, eng::triangle t2)
{
    shader s("/home/apachai/CLionProjects/opengl_window/vertex.vert",
             "/home/apachai/CLionProjects/opengl_window/fragment.frag");

    float        vertices[] = { 1.0f,  1.0f,  0.0f, 1.0f,  -1.0f, 0.0f,
                                -1.0f, -1.0f, 0.0f, -1.0f, 1.0f,  0.0f };
    unsigned int indices[]  = { 0, 1, 3, 1, 2, 3 };
    GLuint       VAO;
    glGenVertexArrays(1, &VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);

    GLuint EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    int vertexTimeLocation  = glGetUniformLocation(ID, "time");
    int vertexColorLocation = glGetUniformLocation(ID, "resol");

    s.use();
    float time = SDL_GetTicks() / 100;
    glUniform1f(vertexTimeLocation, 3.14159 * time / 8);
    glUniform2f(vertexColorLocation, eng::width, eng::height);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    OM_GL_CHECK()
}
bool engine_impl::draw_texture(eng::triangle t1,
                               eng::triangle t2,
                               int           texHandle,
                               glm::mat4     transform)
{

    shader s("vertex.vert", "fragment.frag");

    eng::vertex vertices[] = {
        t1.v[0],
        t1.v[1],
        t1.v[2],
        t2.v[0],
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          8 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    s.use();

    glBindTexture(GL_TEXTURE_2D, texHandle);
    glActiveTexture(GL_TEXTURE0);
    s.setInt("ourTexture", 0);
    s.setMat4("transform", transform);
    OM_GL_CHECK()
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    return true;
}

engine* create_engine()
{
    if (already_exist)
    {
        throw std::runtime_error("engine already exist");
    }
    engine* result = new engine_impl();
    already_exist  = true;
    return result;
}

void destroy_engine(engine* e)
{
    if (already_exist == false)
    {
        throw std::runtime_error("engine not created");
    }
    if (nullptr == e)
    {
        throw std::runtime_error("e is nullptr");
    }
    delete e;
}
bool engine_impl::get_input(eng::event& e)
{
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            std::cout << "Goodbye :) " << std::endl;
            atexit(SDL_Quit);
            exit(0);
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            auto it =
                std::find_if(binded_keys.begin(),
                             binded_keys.end(),
                             [&](const CKeys& k)
                             { return k.get_code() == event.key.keysym.sym; });
            if (it == binded_keys.end())
            {
                return true;
            }
            e = it->get_event();
            std::cout << it->get_name() << " Key Down" << std::endl;
            return true;
        }
        if (event.type == SDL_EVENT_KEY_UP)
        {
            auto it =
                std::find_if(binded_keys.begin(),
                             binded_keys.end(),
                             [&](const CKeys& k)
                             { return k.get_code() == event.key.keysym.sym; });
            if (it == binded_keys.end())
            {
                return true;
            }
            e = it->get_event();
            std::cout << it->get_name() << " Key Released" << std::endl;
            return true;
        }
    }
    return false;
}
static const char* source_to_strv(GLenum source)
{
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "SHADER_COMPILER";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "OTHER";
    }
    return "unknown";
}

static const char* type_to_strv(GLenum type)
{
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";
    }
    return "unknown";
}

static const char* severity_to_strv(GLenum severity)
{
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:
            return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "NOTIFICATION";
    }
    return "unknown";
}

static std::array<char, GL_MAX_DEBUG_MESSAGE_LENGTH> local_log_buff;

static void APIENTRY
callback_opengl_debug(GLenum                       source,
                      GLenum                       type,
                      GLuint                       id,
                      GLenum                       severity,
                      GLsizei                      length,
                      const GLchar*                message,
                      [[maybe_unused]] const void* userParam)
{
    auto& buff{ local_log_buff };
    int   num_chars = std::snprintf(buff.data(),
                                  buff.size(),
                                  "%s %s %d %s %.*s\n",
                                  source_to_strv(source),
                                  type_to_strv(type),
                                  id,
                                  severity_to_strv(severity),
                                  length,
                                  message);

    if (num_chars > 0)
    {

        std::cerr.write(buff.data(), num_chars);
    }
}
} // namespace eng