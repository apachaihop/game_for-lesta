#include "glad/glad.h"
#include "text_renderer.hxx"
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <string>

#ifdef __ANDROID__
#include <GLES3/gl32.h>
#include <android/log.h>
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL_stdinc.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

#include "imgui/imgui.h"
#include "stb_image.h"

#include "engine.hxx"

#ifdef __WIN32
#define GLES_MAJOR_VERSION 3
#define GLES_MINOR_VERSION 0
#else
#define GLES_MAJOR_VERSION 3
#define GLES_MINOR_VERSION 2
#endif

struct ImGui_ImplSDL3_Data
{
    SDL_Window* Window;
    Uint64      Time;
    Uint32      MouseWindowID;
    int         MouseButtonsDown;
    SDL_Cursor* MouseCursors[ImGuiMouseCursor_COUNT];
    SDL_Cursor* LastMouseCursor;
    int         PendingMouseLeaveFrame;
    char*       ClipboardTextData;
    bool        MouseCanUseGlobalState;

    ImGui_ImplSDL3_Data()
    {
        memset((void*)this, 0, sizeof(*this));
    }
};

struct ImGui_ImplOpenGL3_Data
{
    GLuint GlVersion;

    GLuint       FontTexture;
    GLuint       ShaderHandle;
    GLint        AttribLocationTex;    // Uniforms location
    GLint        AttribLocationProjMtx;
    GLuint       AttribLocationVtxPos; // Vertex attributes location
    GLuint       AttribLocationVtxUV;
    GLuint       AttribLocationVtxColor;
    unsigned int VboHandle, ElementsHandle;
    GLsizeiptr   VertexBufferSize;
    GLsizeiptr   IndexBufferSize;
    bool         HasClipOrigin;
    bool         UseBufferSubData;

    ImGui_ImplOpenGL3_Data()
    {
        memset((void*)this, 0, sizeof(*this));
    }
};

bool ImGui_ImplSdl_Init(SDL_Window* window);

bool ImGui_ImplOpenGL_Init();

void ImGui_ImplSdlGL3_Shutdown();

void ImGui_ImplSdl_NewFrame();

bool ImGui_ImplSdlGL3_ProcessEvent(const SDL_Event* event);

void ImGui_ImplOpenGL_RenderDrawData(ImDrawData* draw_data);
bool ImGuiImplOpenGLCreateDeviceObjects();
void ImGui_ImplOpenGL_DestroyDeviceObjects();

static ImGui_ImplOpenGL3_Data* ImGui_ImplOpenGL3_GetBackendData()
{
    return ImGui::GetCurrentContext()
               ? (ImGui_ImplOpenGL3_Data*)ImGui::GetIO().BackendRendererUserData
               : nullptr;
}

void ImGui_ImplOpenGL3_NewFrame()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOpenGL3_Init()?");

    if (!bd->ShaderHandle)
        ImGuiImplOpenGLCreateDeviceObjects();
}
bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO&                io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int            width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->FontTexture);
    glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    ImGuiIO&                io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->FontTexture)
    {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

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
    CKeys()
    {
    }
    CKeys(SDL_KeyCode k, std::string n, enum event e)
        : key(k)
        , name(n)
        , ev(e)
    {
    }
    SDL_KeyCode get_code() const
    {
        return this->key;
    }
    std::string get_name()
    {
        return this->name;
    }
    enum event get_event()
    {
        return ev;
    }
};
static std::string_view get_sound_format_name(uint16_t format_value)
{
    static const std::map<int, std::string_view> format = {
        { SDL_AUDIO_U8, "AUDIO_U8" },
        { SDL_AUDIO_S8, "AUDIO_S8" },
        { SDL_AUDIO_S16LSB, "AUDIO_S16LSB" },
        { SDL_AUDIO_S16MSB, "AUDIO_S16MSB" },
        { SDL_AUDIO_S32LSB, "AUDIO_S32LSB" },
        { SDL_AUDIO_S32MSB, "AUDIO_S32MSB" },
        { SDL_AUDIO_F32LSB, "AUDIO_F32LSB" },
        { SDL_AUDIO_F32MSB, "AUDIO_F32MSB" },
    };

    auto it = format.find(format_value);
    return it->second;
}

static std::size_t get_sound_format_size(uint16_t format_value)
{
    static const std::map<int, std::size_t> format = {
        { SDL_AUDIO_U8, 1 },
        { SDL_AUDIO_S8, 1 },
        { SDL_AUDIO_S16LSB, 2 },
        { SDL_AUDIO_S16MSB, 2 },
        { SDL_AUDIO_S32LSB, 4 },
        { SDL_AUDIO_S32MSB, 4 },
        { SDL_AUDIO_F32LSB, 4 },
        { SDL_AUDIO_F32MSB, 4 },
    };

    auto it = format.find(format_value);
    return it->second;
}

sound_buffer::~sound_buffer()
{
}
class sound_buffer_impl;

class engine_impl final : public eng::engine
{
    static void       audio_callback(void*, uint8_t*, int);
    static std::mutex audio_mutex;

    friend class sound_buffer_impl;

    SDL_Window*                     window  = nullptr;
    SDL_GLContext                   context = nullptr;
    std::string                     flag;
    std::vector<CKeys>              binded_keys;
    unsigned int                    ID;
    bool                            show_demo_window    = true;
    bool                            show_another_window = false;
    SDL_AudioDeviceID               audio_device;
    SDL_AudioSpec                   audio_device_spec;
    std::vector<sound_buffer_impl*> sounds;

public:
    bool          initialize_engine() final;
    bool          get_input(event& e) final;
    bool          rebind_key() final;
    Collision     check_collision(ball_object& ball, game_object& obj) final;
    Collision check_collision(ball_object& ball, wind_object& obj) final;
    Collision     check_collision(ball_object& ball) final;
    Collision     check_collision(text_renderer& text) final;
    sound_buffer* create_sound_buffer(std::string_view path) final;
    void          destroy_sound_buffer(sound_buffer* sound) final
    {
        delete sound;
    }
    Direction VectorDirection(glm::vec2 target)
    {
        glm::vec2 compass[] = {
            glm::vec2(0.0f, 1.0f),  // up
            glm::vec2(1.0f, 0.0f),  // right
            glm::vec2(0.0f, -1.0f), // down
            glm::vec2(-1.0f, 0.0f)  // left
        };
        float        max        = 0.0f;
        unsigned int best_match = -1;
        for (unsigned int i = 0; i < 4; i++)
        {
            float dot_product = glm::dot(glm::normalize(target), compass[i]);
            if (dot_product > max)
            {
                max        = dot_product;
                best_match = i;
            }
        }
        return (Direction)best_match;
    }
    bool swap_buff() final
    {
        if (dev_mode)
        {
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSdl_NewFrame();
            ImGui::NewFrame();

            {

                float x, y;
                x       = ImGui::GetMousePos().x;
                y       = ImGui::GetMousePos().y;
                mouse_X = x;
                mouse_Y = y;
                ImGui::Begin(
                    "DEVELOP WINDOW"); // Create a window called "Hello,
                                       // world!" and append into it.

                ImGui::Text("X = %f", x);
                ImGui::Text("Y = %f", y);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                            1000.0f / io.Framerate,
                            io.Framerate);
                ImGui::End();
            }

            // Rendering
            ImGui::Render();

            OM_GL_CHECK()
            ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());
        }
        OM_GL_CHECK()
        SDL_GL_SwapWindow(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(96.0f / 255, 205.0f / 255, 52.0f / 255, 0.9f);
        return true;
    }
};
membuf load(std::string path)
{
    SDL_RWops* io = SDL_RWFromFile(path.data(), "rb");
    if (nullptr == io)
    {
        throw std::runtime_error("can't load_file file: " + std::string(path));
    }

    Sint64 file_size = io->size(io);
    if (-1 == file_size)
    {
        throw std::runtime_error("can't determine size of file: " +
                                 std::string(path));
    }
    const size_t            size = static_cast<size_t>(file_size);
    std::unique_ptr<char[]> mem  = std::make_unique<char[]>(size);

    const size_t num_readed_objects = io->read(io, mem.get(), size);
    if (num_readed_objects != size)
    {
        throw std::runtime_error("can't read all content from file: " +
                                 std::string(path));
    }

    if (0 != io->close(io))
    {
        throw std::runtime_error("failed close file: " + std::string(path));
    }
    return membuf(std::move(mem), size);
}
bool engine_impl::rebind_key()
{
    std::cout << "Choose key to rebind" << std::endl;
    std::string key_name;
    std::cin >> key_name;
    auto it = std::find_if(binded_keys.begin(),
                           binded_keys.end(),
                           [&](CKeys& k)
                           { return k.get_name() == key_name; });
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
    if (SDL_Init(SDL_INIT_AUDIO))
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Error", "Cannot init SDL .", NULL);

        return false;
    }

    TTF_Init();

    atexit(SDL_Quit);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "0");
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifdef __ANDROID__

    const SDL_DisplayMode* dispale_mode = SDL_GetCurrentDisplayMode(1);
    if (!dispale_mode)
    {
        std::cout << "can't get current display mode: " << SDL_GetError()
                  << std::endl;
    }
    width_a  = dispale_mode->w;
    height_a = dispale_mode->h;
    window   = SDL_CreateWindow("game", width_a, height_a, SDL_WINDOW_OPENGL);
#else
    width_a  = eng::width;
    height_a = eng::height;
    window   = SDL_CreateWindow("game", width_a, height_a, SDL_WINDOW_OPENGL);
#endif

    if (!window)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), NULL);
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

    glEnable(GL_BLEND);
    OM_GL_CHECK();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OM_GL_CHECK();
    glViewport(0, 0, width_a, height_a);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSdl_Init(window);
    ImGui_ImplOpenGL_Init();

        audio_device_spec.freq     = 48000;
        audio_device_spec.format   = SDL_AUDIO_S16LSB;
        audio_device_spec.channels = 2;
        audio_device_spec.samples  = 1024; // must be power of 2
        audio_device_spec.callback = engine_impl::audio_callback;
        audio_device_spec.userdata = this;

        const int num_audio_drivers = SDL_GetNumAudioDrivers();
        for (int i = 0; i < num_audio_drivers; ++i)
        {
            std::cout << "audio_driver #:" << i << " " <<
            SDL_GetAudioDriver(i)
                      << '\n';
        }
        std::cout << std::flush;
        SDL_setenv("SDL_AUDIO_DRIVER", "pipewire", 1);

        const char* default_audio_device_name = nullptr;

        const int num_audio_devices = SDL_GetNumAudioDevices(SDL_FALSE);
        if (num_audio_devices > 0)
        {
            default_audio_device_name =
                SDL_GetAudioDeviceName(num_audio_devices - 1, SDL_FALSE);
            for (int i = 0; i < num_audio_devices; ++i)
            {
                std::cout << "audio device #" << i << ": "
                          << SDL_GetAudioDeviceName(i, SDL_FALSE) << '\n';
            }
        }
        std::cout << std::flush;

        audio_device = SDL_OpenAudioDevice(default_audio_device_name,
                                           0,
                                           &audio_device_spec,
                                           nullptr,
                                           SDL_AUDIO_ALLOW_ANY_CHANGE);

        if (audio_device == 0)
        {
            std::cerr << "failed open audio device: " << SDL_GetError();
            throw std::runtime_error("audio failed");
        }

        SDL_PlayAudioDevice(audio_device);
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
Collision engine_impl::check_collision(ball_object& ball)
{
    glm::vec2 center(glm::vec2(width_a/2,height_a/2) + ball.radius);
    glm::vec2 difference = glm::vec2(mouse_X, mouse_Y) - center;
#ifdef __ANDROID__
    if (glm::length(difference) < ball.radius * 4)
    {
        return std::make_tuple(true, UP, difference);
    }
    {
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
    }
#endif
    if (glm::length(difference) < ball.radius * 2)
    {
        return std::make_tuple(true, UP, difference);
    }
    else
    {
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
    }
}
Collision engine_impl::check_collision(text_renderer& text)
{
    glm::vec2 center(text.position + glm::vec2(text.size.x/2,text.size.y/2));
    glm::vec2 difference = glm::vec2(mouse_X, mouse_Y) - center;
#ifdef __ANDROID__
    if (glm::length(difference) < 2*text.size.x||glm::length(difference) < 2*text.size.y)
    {
        return std::make_tuple(true, UP, difference);
    }
    {
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
    }
#endif
    if (glm::length(difference) < text.size.x||glm::length(difference) < text.size.y)
    {
        return std::make_tuple(true, UP, difference);
    }
    else
    {
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
    }
}
Collision engine_impl::check_collision(ball_object& ball, game_object& obj)
{ // get center point circle first
    glm::vec2 center(ball.position + ball.radius);
    // calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(obj.size.x / 2.0f, obj.size.y / 2.0f);
    glm::vec2 aabb_center(obj.position.x + aabb_half_extents.x,
                          obj.position.y + aabb_half_extents.y);
    // get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped =
        glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // now that we know the clamped values, add this to AABB_center and we get
    // the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // now retrieve vector between center circle and closest point AABB and
    // check if length < radius
    difference = closest - center;

    if (glm::length(difference) <
        ball.radius) // not <= since in that case a collision also occurs when
                     // object one exactly touches object two, which they are at
                     // the end of each collision resolution stage.
        return std::make_tuple(true, VectorDirection(difference), difference);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}
Collision engine_impl::check_collision(ball_object& ball, wind_object& obj) {
    glm::vec2 center(ball.position + ball.radius);
    if(center.x>obj.start_position.x-90&&center.x<obj.start_position.x+64)
    {
        return std::make_tuple(true, UP, glm::vec2(0.0f, 0.0f));
    }
    else return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

class sound_buffer_impl final : public sound_buffer
{
public:
    sound_buffer_impl(std::string_view  path,
                      SDL_AudioDeviceID device,
                      SDL_AudioSpec     audio_spec);
    ~sound_buffer_impl() final;

    void play(const properties prop) final
    {
        std::lock_guard<std::mutex> lock(engine_impl::audio_mutex);

        current_index = 0;
        is_playing    = true;
        is_looped     = (prop == properties::looped);
    }

    std::unique_ptr<uint8_t[]> tmp_buf;
    uint8_t*                   buffer;
    uint32_t                   length;
    uint32_t                   current_index = 0;
    SDL_AudioDeviceID          device;
    bool                       is_playing = false;
    bool                       is_looped  = false;
};
#pragma pack(pop)

sound_buffer_impl::sound_buffer_impl(std::string_view  path,
                                     SDL_AudioDeviceID device_,
                                     SDL_AudioSpec     device_audio_spec)
    : buffer(nullptr)
    , length(0)
    , device(device_)
{
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (file == nullptr)
    {
        throw std::runtime_error(std::string("can't open audio file: ") +
                                 path.data());
    }

    // freq, format, channels, and samples - used by SDL_LoadWAV_RW
    SDL_AudioSpec file_audio_spec;

    if (nullptr == SDL_LoadWAV_RW(file, 1, &file_audio_spec, &buffer, &length))
    {
        throw std::runtime_error(std::string("can't load_file wav: ") +
                                 path.data());
    }

    std::cout << "--------------------------------------------\n";
    std::cout << "audio format for: " << path << '\n'
              << "format: " << get_sound_format_name(file_audio_spec.format)
              << '\n'
              << "sample_size: "
              << get_sound_format_size(file_audio_spec.format) << '\n'
              << "channels: " << static_cast<uint32_t>(file_audio_spec.channels)
              << '\n'
              << "frequency: " << file_audio_spec.freq << '\n'
              << "length: " << length << '\n'
              << "time: "
              << static_cast<double>(length) /
                     (file_audio_spec.channels *
                      static_cast<uint32_t>(file_audio_spec.freq) *
                      get_sound_format_size(file_audio_spec.format))
              << "sec" << std::endl;
    std::cout << "--------------------------------------------\n";

    if (file_audio_spec.channels != device_audio_spec.channels ||
        file_audio_spec.format != device_audio_spec.format ||
        file_audio_spec.freq != device_audio_spec.freq)
    {
        Uint8* output_bytes;
        int    output_length;

        int convert_status = SDL_ConvertAudioSamples(file_audio_spec.format,
                                                     file_audio_spec.channels,
                                                     file_audio_spec.freq,
                                                     buffer,
                                                     static_cast<int>(length),
                                                     device_audio_spec.format,
                                                     device_audio_spec.channels,
                                                     device_audio_spec.freq,
                                                     &output_bytes,
                                                     &output_length);
        if (0 != convert_status)
        {
            std::stringstream message;
            message << "failed to convert WAV byte stream: " << SDL_GetError();
            throw std::runtime_error(message.str());
        }

        SDL_free(buffer);
        buffer = output_bytes;
        length = static_cast<uint32_t>(output_length);
    }
    else
    {
        // no need to convert buffer, use as is
    }
}

sound_buffer_impl::~sound_buffer_impl()
{
    if (!tmp_buf)
    {
        SDL_free(buffer);
    }
    buffer = nullptr;
    length = 0;
}

sound_buffer* engine_impl::create_sound_buffer(std::string_view path)
{
    sound_buffer_impl* s =
        new sound_buffer_impl(path, audio_device, audio_device_spec);
    {
        // push_backsound_buffer_impl
        std::lock_guard<std::mutex> lock(audio_mutex);
        sounds.push_back(s);
    }
    return s;
}

std::mutex engine_impl::audio_mutex;

void engine_impl::audio_callback(void*    engine_ptr,
                                 uint8_t* stream,
                                 int      stream_size)
{
    std::lock_guard<std::mutex> lock(audio_mutex);
    // no sound default
    std::fill_n(stream, stream_size, '\0');

    engine_impl* e = static_cast<engine_impl*>(engine_ptr);

    for (sound_buffer_impl* snd : e->sounds)
    {
        if (snd->is_playing)
        {
            uint32_t rest         = snd->length - snd->current_index;
            uint8_t* current_buff = &snd->buffer[snd->current_index];

            if (rest <= static_cast<uint32_t>(stream_size))
            {
                // copy rest to buffer
                SDL_MixAudioFormat(stream,
                                   current_buff,
                                   e->audio_device_spec.format,
                                   rest,
                                   SDL_MIX_MAXVOLUME);
                snd->current_index += rest;
            }
            else
            {
                SDL_MixAudioFormat(stream,
                                   current_buff,
                                   e->audio_device_spec.format,
                                   static_cast<uint32_t>(stream_size),
                                   snd->voulume);
                snd->current_index += static_cast<uint32_t>(stream_size);
            }

            if (snd->current_index == snd->length)
            {
                if (snd->is_looped)
                {
                    // start from begining
                    snd->current_index = 0;
                }
                else
                {
                    snd->is_playing = false;
                }
            }
        }
    }
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
// namespace eng
static ImGui_ImplSDL3_Data* ImGui_ImplSDL3_GetBackendData()
{
    return ImGui::GetCurrentContext()
               ? (ImGui_ImplSDL3_Data*)ImGui::GetIO().BackendPlatformUserData
               : nullptr;
}

// Functions
static const char* ImGui_ImplSDL3_GetClipboardText(void*)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    if (bd->ClipboardTextData)
        SDL_free(bd->ClipboardTextData);
    bd->ClipboardTextData = SDL_GetClipboardText();
    return bd->ClipboardTextData;
}

static void ImGui_ImplSDL3_SetClipboardText(void*, const char* text)
{
    SDL_SetClipboardText(text);
}

static void ImGui_ImplSDL3_SetPlatformImeData(ImGuiViewport*,
                                              ImGuiPlatformImeData* data)
{
    if (data->WantVisible)
    {
        SDL_Rect r;
        r.x = (int)data->InputPos.x;
        r.y = (int)data->InputPos.y;
        r.w = 1;
        r.h = (int)data->InputLineHeight;
        SDL_SetTextInputRect(&r);
    }
}

bool ImGui_ImplSdl_Init(SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();

    bool mouse_can_use_global_state = false;

    // Setup backend capabilities flags
    ImGui_ImplSDL3_Data* bd    = IM_NEW(ImGui_ImplSDL3_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName     = "imgui_impl_sdl3";
    io.BackendFlags |=
        ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor()
                                           // values (optional)
    io.BackendFlags |=
        ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos
                                           // requests (optional, rarely used)

    bd->Window                 = window;
    bd->MouseCanUseGlobalState = mouse_can_use_global_state;

    io.SetClipboardTextFn   = ImGui_ImplSDL3_SetClipboardText;
    io.GetClipboardTextFn   = ImGui_ImplSDL3_GetClipboardText;
    io.ClipboardUserData    = nullptr;
    io.SetPlatformImeDataFn = ImGui_ImplSDL3_SetPlatformImeData;

    // Load mouse cursors
    bd->MouseCursors[ImGuiMouseCursor_Arrow] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    bd->MouseCursors[ImGuiMouseCursor_Hand] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main
    // viewport
    ImGuiViewport* main_viewport     = ImGui::GetMainViewport();
    main_viewport->PlatformHandleRaw = nullptr;

#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

// From 2.0.22: Disable auto-capture, this is preventing drag and drop across
// multiple windows (see #5710)
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "1");
#endif

    return true;
}
bool ImGui_ImplOpenGL_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr &&
              "Already initialized a renderer backend!");

    ImGui_ImplOpenGL3_Data* bd = IM_NEW(ImGui_ImplOpenGL3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName     = "imgui_impl_opengl3";

    bd->GlVersion        = (GLuint)(3 * 100 + 2 * 10);
    bd->UseBufferSubData = false;

    if (bd->GlVersion >= 320)
        io.BackendFlags |=
            ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the

    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    bd->HasClipOrigin = (bd->GlVersion >= 450);

    return true;
}

bool ImGuiImplOpenGLCreateDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar* vertex_shader_glsl_300_es =

        "precision highp float;\n"
        "layout (location = 0) in mediump vec2 Position;\n"
        "layout (location = 1) in mediump vec2 UV;\n"
        "layout (location = 2) in mediump vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out mediump vec2 Frag_UV;\n"
        "out mediump vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_300_es =

        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in mediump vec2 Frag_UV;\n"
        "in mediump vec4 Frag_Color;\n"
        "layout (location = 0) out mediump vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar* vertex_shader   = nullptr;
    const GLchar* fragment_shader = nullptr;

    vertex_shader   = vertex_shader_glsl_300_es;
    fragment_shader = fragment_shader_glsl_300_es;

    // Create shaders
    const GLchar* vertex_shader_with_version[2] = { "#version 320 es\n",
                                                    vertex_shader };
    GLuint        vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 2, vertex_shader_with_version, nullptr);
    glCompileShader(vert_handle);

    const GLchar* fragment_shader_with_version[2] = { "#version 320 es\n",
                                                      fragment_shader };
    GLuint        frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 2, fragment_shader_with_version, nullptr);
    glCompileShader(frag_handle);

    // Link
    bd->ShaderHandle = glCreateProgram();
    glAttachShader(bd->ShaderHandle, vert_handle);
    glAttachShader(bd->ShaderHandle, frag_handle);
    glLinkProgram(bd->ShaderHandle);

    glDetachShader(bd->ShaderHandle, vert_handle);
    glDetachShader(bd->ShaderHandle, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->AttribLocationTex = glGetUniformLocation(bd->ShaderHandle, "Texture");
    bd->AttribLocationProjMtx =
        glGetUniformLocation(bd->ShaderHandle, "ProjMtx");
    bd->AttribLocationVtxPos =
        (GLuint)glGetAttribLocation(bd->ShaderHandle, "Position");
    bd->AttribLocationVtxUV =
        (GLuint)glGetAttribLocation(bd->ShaderHandle, "UV");
    bd->AttribLocationVtxColor =
        (GLuint)glGetAttribLocation(bd->ShaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &bd->VboHandle);
    glGenBuffers(1, &bd->ElementsHandle);

    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);
    return true;
}

void ImGui_ImplOpenGL_DestroyDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->VboHandle)
    {
        glDeleteBuffers(1, &bd->VboHandle);
        bd->VboHandle = 0;
    }
    if (bd->ElementsHandle)
    {
        glDeleteBuffers(1, &bd->ElementsHandle);
        bd->ElementsHandle = 0;
    }
    if (bd->ShaderHandle)
    {
        glDeleteProgram(bd->ShaderHandle);
        bd->ShaderHandle = 0;
    }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}
static void ImGui_ImplSDL3_UpdateMouseData()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    ImGuiIO&             io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via
    // SDL_EVENT_MOUSE_MOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the
    // SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse((bd->MouseButtonsDown != 0) ? SDL_TRUE : SDL_FALSE);
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool  is_app_focused = (bd->Window == focused_window);
#else
    SDL_Window* focused_window = bd->Window;
    const bool  is_app_focused =
        (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) !=
        0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif
    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely
        // used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by
        // user)
        if (io.WantSetMousePos)
            SDL_WarpMouseInWindow(bd->Window, io.MousePos.x, io.MousePos.y);

        // (Optional) Fallback to provide mouse position when focused
        // (SDL_EVENT_MOUSE_MOTION already provides this when hovered or
        // captured)
        if (bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0)
        {
            // Single-viewport mode: mouse position in client window coordinates
            // (io.MousePos is (0,0) when the mouse is on the upper-left corner
            // of the app window)
            float mouse_x_global, mouse_y_global;
            int   window_x, window_y;
            SDL_GetGlobalMouseState(&mouse_x_global, &mouse_y_global);
            SDL_GetWindowPosition(focused_window, &window_x, &window_y);
            io.AddMousePosEvent(mouse_x_global - window_x,
                                mouse_y_global - window_y);
        }
    }
}

static void ImGui_ImplSDL3_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_HideCursor();
    }
    else
    {
        // Show OS mouse cursor
        SDL_Cursor* expected_cursor =
            bd->MouseCursors[imgui_cursor]
                ? bd->MouseCursors[imgui_cursor]
                : bd->MouseCursors[ImGuiMouseCursor_Arrow];
        if (bd->LastMouseCursor != expected_cursor)
        {
            SDL_SetCursor(expected_cursor); // SDL function doesn't have an
                                            // early out (see #6113)
            bd->LastMouseCursor = expected_cursor;
        }
        SDL_ShowCursor();
    }
}

static void ImGui_ImplSDL3_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Get gamepad
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    SDL_Gamepad* gamepad = SDL_OpenGamepad(0);
    if (!gamepad)
        return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

// Update gamepad inputs
#define IM_SATURATE(V) (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
#define MAP_BUTTON(KEY_NO, BUTTON_NO)                                          \
    {                                                                          \
        io.AddKeyEvent(KEY_NO, SDL_GetGamepadButton(gamepad, BUTTON_NO) != 0); \
    }
#define MAP_ANALOG(KEY_NO, AXIS_NO, V0, V1)                                    \
    {                                                                          \
        float vn = (float)(SDL_GetGamepadAxis(gamepad, AXIS_NO) - V0) /        \
                   (float)(V1 - V0);                                           \
        vn = IM_SATURATE(vn);                                                  \
        io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn);                           \
    }
    const int thumb_dead_zone =
        8000; // SDL_gamecontroller.h suggests using this value.
    MAP_BUTTON(ImGuiKey_GamepadStart, SDL_GAMEPAD_BUTTON_START);
    MAP_BUTTON(ImGuiKey_GamepadBack, SDL_GAMEPAD_BUTTON_BACK);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,
               SDL_GAMEPAD_BUTTON_X); // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,
               SDL_GAMEPAD_BUTTON_B); // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,
               SDL_GAMEPAD_BUTTON_Y); // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,
               SDL_GAMEPAD_BUTTON_A); // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp, SDL_GAMEPAD_BUTTON_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadL1, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    MAP_ANALOG(ImGuiKey_GamepadL2, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0.0f, 32767);
    MAP_ANALOG(ImGuiKey_GamepadR2, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.0f, 32767);
    MAP_BUTTON(ImGuiKey_GamepadL3, SDL_GAMEPAD_BUTTON_LEFT_STICK);
    MAP_BUTTON(ImGuiKey_GamepadR3, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,
               SDL_GAMEPAD_AXIS_LEFTX,
               -thumb_dead_zone,
               -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,
               SDL_GAMEPAD_AXIS_LEFTX,
               +thumb_dead_zone,
               +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,
               SDL_GAMEPAD_AXIS_LEFTY,
               -thumb_dead_zone,
               -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,
               SDL_GAMEPAD_AXIS_LEFTY,
               +thumb_dead_zone,
               +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,
               SDL_GAMEPAD_AXIS_RIGHTX,
               -thumb_dead_zone,
               -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,
               SDL_GAMEPAD_AXIS_RIGHTX,
               +thumb_dead_zone,
               +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,
               SDL_GAMEPAD_AXIS_RIGHTY,
               -thumb_dead_zone,
               -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,
               SDL_GAMEPAD_AXIS_RIGHTY,
               +thumb_dead_zone,
               +32767);
#undef MAP_BUTTON
#undef MAP_ANALOG
}

void ImGui_ImplSdl_NewFrame()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplSDL3_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(bd->Window, &w, &h);
    if (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    SDL_GetWindowSizeInPixels(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale =
            ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step (we don't use SDL_GetTicks() because it is using
    // millisecond resolution) (Accept SDL_GetPerformanceCounter() not returning
    // a monotonically increasing value. Happens in VMs and Emscripten, see
    // #6189, #6114, #3644)
    static Uint64 frequency    = SDL_GetPerformanceFrequency();
    Uint64        current_time = SDL_GetPerformanceCounter();
    if (current_time <= bd->Time)
        current_time = bd->Time + 1;
    io.DeltaTime = bd->Time > 0
                       ? (float)((double)(current_time - bd->Time) / frequency)
                       : (float)(1.0f / 60.0f);
    bd->Time     = current_time;

    if (bd->PendingMouseLeaveFrame &&
        bd->PendingMouseLeaveFrame >= ImGui::GetFrameCount() &&
        bd->MouseButtonsDown == 0)
    {
        bd->MouseWindowID          = 0;
        bd->PendingMouseLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    ImGui_ImplSDL3_UpdateMouseData();
    ImGui_ImplSDL3_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplSDL3_UpdateGamepads();
}
static void ImGui_ImplOpenGL_SetupRenderState(ImDrawData* draw_data,
                                              int         fb_width,
                                              int         fb_height,
                                              GLuint      vertex_array_object)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth
    // testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(
        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos
    // is (0,0) for single viewport apps.
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#if defined(GL_CLIP_ORIGIN)
    if (!clip_origin_lower_left)
    {
        float tmp = T;
        T         = B;
        B         = tmp;
    } // Swap top and bottom if origin is upper left
#endif
    const float ortho_projection[4][4] = {
        { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f },
    };
    glUseProgram(bd->ShaderHandle);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(
        bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

    (void)vertex_array_object;

    glBindVertexArray(vertex_array_object);

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
    glEnableVertexAttribArray(bd->AttribLocationVtxPos);
    glEnableVertexAttribArray(bd->AttribLocationVtxUV);
    glEnableVertexAttribArray(bd->AttribLocationVtxColor);
    glVertexAttribPointer(bd->AttribLocationVtxPos,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(ImDrawVert),
                          (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(bd->AttribLocationVtxUV,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(ImDrawVert),
                          (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(bd->AttribLocationVtxColor,
                          4,
                          GL_UNSIGNED_BYTE,
                          GL_TRUE,
                          sizeof(ImDrawVert),
                          (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
}
void ImGui_ImplOpenGL_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays
    // (screen coordinates != framebuffer coordinates)
    int fb_width =
        (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height =
        (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLuint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&last_program);
    GLuint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&last_texture);

    GLuint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&last_array_buffer);

    GLint last_element_array_buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);

    GLuint last_vertex_array_object;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&last_vertex_array_object);

    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb;
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_enable_blend        = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face    = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test   = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts
    // to be rendered to. VAO are not shared among GL contexts) The renderer
    // would actually work without any VAO bound, but then our VertexAttrib
    // calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;

    glGenVertexArrays(1, &vertex_array_object);

    ImGui_ImplOpenGL_SetupRenderState(
        draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off =
        draw_data->DisplayPos;       // (0,0) unless using multi-viewports
    ImVec2 clip_scale =
        draw_data->FramebufferScale; // (1,1) unless using retina display which
                                     // are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        // - OpenGL drivers are in a very sorry state nowadays....
        //   During 2021 we attempted to switch from glBufferData() to
        //   orphaning+glBufferSubData() following reports of leaks on Intel GPU
        //   when using multi-viewports on Windows.
        // - After this we kept hearing of various display corruptions issues.
        // We started disabling on non-Intel GPU, but issues still got reported
        // on Intel.
        // - We are now back to using exclusively glBufferData(). So
        // bd->UseBufferSubData IS ALWAYS FALSE in this code.
        //   We are keeping the old code path for a while in case people finding
        //   new issues may want to test the bd->UseBufferSubData path.
        // - See https://github.com/ocornut/imgui/issues/4468 and please report
        // any corruption issues.
        const GLsizeiptr vtx_buffer_size =
            (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        const GLsizeiptr idx_buffer_size =
            (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        if (bd->UseBufferSubData)
        {
            if (bd->VertexBufferSize < vtx_buffer_size)
            {
                bd->VertexBufferSize = vtx_buffer_size;
                glBufferData(GL_ARRAY_BUFFER,
                             bd->VertexBufferSize,
                             nullptr,
                             GL_STREAM_DRAW);
            }
            if (bd->IndexBufferSize < idx_buffer_size)
            {
                bd->IndexBufferSize = idx_buffer_size;
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             bd->IndexBufferSize,
                             nullptr,
                             GL_STREAM_DRAW);
            }
            glBufferSubData(GL_ARRAY_BUFFER,
                            0,
                            vtx_buffer_size,
                            (const GLvoid*)cmd_list->VtxBuffer.Data);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                            0,
                            idx_buffer_size,
                            (const GLvoid*)cmd_list->IdxBuffer.Data);
        }
        else
        {
            glBufferData(GL_ARRAY_BUFFER,
                         vtx_buffer_size,
                         (const GLvoid*)cmd_list->VtxBuffer.Data,
                         GL_STREAM_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         idx_buffer_size,
                         (const GLvoid*)cmd_list->IdxBuffer.Data,
                         GL_STREAM_DRAW);
        }

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value
                // used by the user to request the renderer to reset render
                // state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL_SetupRenderState(
                        draw_data, fb_width, fb_height, vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glScissor((int)clip_min.x,
                          (int)((float)fb_height - clip_max.y),
                          (int)(clip_max.x - clip_min.x),
                          (int)(clip_max.y - clip_min.y));

                // Bind texture, Draw
                glBindTexture(GL_TEXTURE_2D,
                              (GLuint)(intptr_t)pcmd->GetTexID());

                glDrawElements(
                    GL_TRIANGLES,
                    (GLsizei)pcmd->ElemCount,
                    sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                           : GL_UNSIGNED_INT,
                    (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    glDeleteVertexArrays(1, &vertex_array_object);

    // Restore modified GL state
    // This "glIsProgram()" check is required because if the program is "pending
    // deletion" at the time of binding backup, it will have been deleted by now
    // and will cause an OpenGL error. See #6220.
    if (last_program == 0 || glIsProgram(last_program))
        glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);

    glActiveTexture(last_active_texture);

    glBindVertexArray(last_vertex_array_object);

    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);

    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb,
                        last_blend_dst_rgb,
                        last_blend_src_alpha,
                        last_blend_dst_alpha);
    if (last_enable_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (last_enable_cull_face)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (last_enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (last_enable_stencil_test)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    if (last_enable_scissor_test)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    glViewport(last_viewport[0],
               last_viewport[1],
               (GLsizei)last_viewport[2],
               (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0],
              last_scissor_box[1],
              (GLsizei)last_scissor_box[2],
              (GLsizei)last_scissor_box[3]);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    (void)bd; // Not all compilation paths use this
}

static ImGuiKey ImGui_ImplSDL3_KeycodeToImGuiKey(int keycode)
{
    switch (keycode)
    {
        case SDLK_TAB:
            return ImGuiKey_Tab;
        case SDLK_LEFT:
            return ImGuiKey_LeftArrow;
        case SDLK_RIGHT:
            return ImGuiKey_RightArrow;
        case SDLK_UP:
            return ImGuiKey_UpArrow;
        case SDLK_DOWN:
            return ImGuiKey_DownArrow;
        case SDLK_PAGEUP:
            return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN:
            return ImGuiKey_PageDown;
        case SDLK_HOME:
            return ImGuiKey_Home;
        case SDLK_END:
            return ImGuiKey_End;
        case SDLK_INSERT:
            return ImGuiKey_Insert;
        case SDLK_DELETE:
            return ImGuiKey_Delete;
        case SDLK_BACKSPACE:
            return ImGuiKey_Backspace;
        case SDLK_SPACE:
            return ImGuiKey_Space;
        case SDLK_RETURN:
            return ImGuiKey_Enter;
        case SDLK_ESCAPE:
            return ImGuiKey_Escape;
        case SDLK_QUOTE:
            return ImGuiKey_Apostrophe;
        case SDLK_COMMA:
            return ImGuiKey_Comma;
        case SDLK_MINUS:
            return ImGuiKey_Minus;
        case SDLK_PERIOD:
            return ImGuiKey_Period;
        case SDLK_SLASH:
            return ImGuiKey_Slash;
        case SDLK_SEMICOLON:
            return ImGuiKey_Semicolon;
        case SDLK_EQUALS:
            return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET:
            return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH:
            return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET:
            return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE:
            return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK:
            return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK:
            return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR:
            return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN:
            return ImGuiKey_PrintScreen;
        case SDLK_PAUSE:
            return ImGuiKey_Pause;
        case SDLK_KP_0:
            return ImGuiKey_Keypad0;
        case SDLK_KP_1:
            return ImGuiKey_Keypad1;
        case SDLK_KP_2:
            return ImGuiKey_Keypad2;
        case SDLK_KP_3:
            return ImGuiKey_Keypad3;
        case SDLK_KP_4:
            return ImGuiKey_Keypad4;
        case SDLK_KP_5:
            return ImGuiKey_Keypad5;
        case SDLK_KP_6:
            return ImGuiKey_Keypad6;
        case SDLK_KP_7:
            return ImGuiKey_Keypad7;
        case SDLK_KP_8:
            return ImGuiKey_Keypad8;
        case SDLK_KP_9:
            return ImGuiKey_Keypad9;
        case SDLK_KP_PERIOD:
            return ImGuiKey_KeypadDecimal;
        case SDLK_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case SDLK_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case SDLK_KP_MINUS:
            return ImGuiKey_KeypadSubtract;
        case SDLK_KP_PLUS:
            return ImGuiKey_KeypadAdd;
        case SDLK_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case SDLK_KP_EQUALS:
            return ImGuiKey_KeypadEqual;
        case SDLK_LCTRL:
            return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT:
            return ImGuiKey_LeftShift;
        case SDLK_LALT:
            return ImGuiKey_LeftAlt;
        case SDLK_LGUI:
            return ImGuiKey_LeftSuper;
        case SDLK_RCTRL:
            return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT:
            return ImGuiKey_RightShift;
        case SDLK_RALT:
            return ImGuiKey_RightAlt;
        case SDLK_RGUI:
            return ImGuiKey_RightSuper;
        case SDLK_APPLICATION:
            return ImGuiKey_Menu;
        case SDLK_0:
            return ImGuiKey_0;
        case SDLK_1:
            return ImGuiKey_1;
        case SDLK_2:
            return ImGuiKey_2;
        case SDLK_3:
            return ImGuiKey_3;
        case SDLK_4:
            return ImGuiKey_4;
        case SDLK_5:
            return ImGuiKey_5;
        case SDLK_6:
            return ImGuiKey_6;
        case SDLK_7:
            return ImGuiKey_7;
        case SDLK_8:
            return ImGuiKey_8;
        case SDLK_9:
            return ImGuiKey_9;
        case SDLK_a:
            return ImGuiKey_A;
        case SDLK_b:
            return ImGuiKey_B;
        case SDLK_c:
            return ImGuiKey_C;
        case SDLK_d:
            return ImGuiKey_D;
        case SDLK_e:
            return ImGuiKey_E;
        case SDLK_f:
            return ImGuiKey_F;
        case SDLK_g:
            return ImGuiKey_G;
        case SDLK_h:
            return ImGuiKey_H;
        case SDLK_i:
            return ImGuiKey_I;
        case SDLK_j:
            return ImGuiKey_J;
        case SDLK_k:
            return ImGuiKey_K;
        case SDLK_l:
            return ImGuiKey_L;
        case SDLK_m:
            return ImGuiKey_M;
        case SDLK_n:
            return ImGuiKey_N;
        case SDLK_o:
            return ImGuiKey_O;
        case SDLK_p:
            return ImGuiKey_P;
        case SDLK_q:
            return ImGuiKey_Q;
        case SDLK_r:
            return ImGuiKey_R;
        case SDLK_s:
            return ImGuiKey_S;
        case SDLK_t:
            return ImGuiKey_T;
        case SDLK_u:
            return ImGuiKey_U;
        case SDLK_v:
            return ImGuiKey_V;
        case SDLK_w:
            return ImGuiKey_W;
        case SDLK_x:
            return ImGuiKey_X;
        case SDLK_y:
            return ImGuiKey_Y;
        case SDLK_z:
            return ImGuiKey_Z;
        case SDLK_F1:
            return ImGuiKey_F1;
        case SDLK_F2:
            return ImGuiKey_F2;
        case SDLK_F3:
            return ImGuiKey_F3;
        case SDLK_F4:
            return ImGuiKey_F4;
        case SDLK_F5:
            return ImGuiKey_F5;
        case SDLK_F6:
            return ImGuiKey_F6;
        case SDLK_F7:
            return ImGuiKey_F7;
        case SDLK_F8:
            return ImGuiKey_F8;
        case SDLK_F9:
            return ImGuiKey_F9;
        case SDLK_F10:
            return ImGuiKey_F10;
        case SDLK_F11:
            return ImGuiKey_F11;
        case SDLK_F12:
            return ImGuiKey_F12;
    }
    return ImGuiKey_None;
}

static void ImGui_ImplSDL3_UpdateKeyModifiers(SDL_Keymod sdl_key_mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (sdl_key_mods & SDL_KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & SDL_KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (sdl_key_mods & SDL_KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & SDL_KMOD_GUI) != 0);
}
bool ImGui_ImplSDL3_ProcessEvent(SDL_Event* event)
{
    ImGuiIO&             io = ImGui::GetIO();
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            ImVec2 mouse_pos((float)event->motion.x, (float)event->motion.y);
            io.AddMouseSourceEvent(event->motion.which == SDL_TOUCH_MOUSEID
                                       ? ImGuiMouseSource_TouchScreen
                                       : ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
            return true;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            // IMGUI_DEBUG_LOG("wheel %.2f %.2f, precise %.2f %.2f\n",
            // (float)event->wheel.x, (float)event->wheel.y,
            // event->wheel.preciseX, event->wheel.preciseY);
            float wheel_x = -event->wheel.x;
            float wheel_y = event->wheel.y;
#ifdef __EMSCRIPTEN__
            wheel_x /= 100.0f;
#endif
            io.AddMouseSourceEvent(event->wheel.which == SDL_TOUCH_MOUSEID
                                       ? ImGuiMouseSource_TouchScreen
                                       : ImGuiMouseSource_Mouse);
            io.AddMouseWheelEvent(wheel_x, wheel_y);
            return true;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            int mouse_button = -1;
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                mouse_button = 0;
            }
            if (event->button.button == SDL_BUTTON_RIGHT)
            {
                mouse_button = 1;
            }
            if (event->button.button == SDL_BUTTON_MIDDLE)
            {
                mouse_button = 2;
            }
            if (event->button.button == SDL_BUTTON_X1)
            {
                mouse_button = 3;
            }
            if (event->button.button == SDL_BUTTON_X2)
            {
                mouse_button = 4;
            }
            if (mouse_button == -1)
                break;
            io.AddMouseSourceEvent(event->button.which == SDL_TOUCH_MOUSEID
                                       ? ImGuiMouseSource_TouchScreen
                                       : ImGuiMouseSource_Mouse);
            io.AddMouseButtonEvent(
                mouse_button, (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN));
            bd->MouseButtonsDown =
                (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    ? (bd->MouseButtonsDown | (1 << mouse_button))
                    : (bd->MouseButtonsDown & ~(1 << mouse_button));
            return true;
        }
        case SDL_EVENT_TEXT_INPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            ImGui_ImplSDL3_UpdateKeyModifiers(
                (SDL_Keymod)event->key.keysym.mod);
            ImGuiKey key =
                ImGui_ImplSDL3_KeycodeToImGuiKey(event->key.keysym.sym);
            io.AddKeyEvent(key, (event->type == SDL_EVENT_KEY_DOWN));
            io.SetKeyEventNativeData(
                key,
                event->key.keysym.sym,
                event->key.keysym.scancode,
                event->key.keysym
                    .scancode); // To support legacy indexing (<1.87 user code).
                                // Legacy backend uses SDLK_*** as indices to
                                // IsKeyXXX() functions.
            return true;
        }
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        {
            bd->MouseWindowID          = event->window.windowID;
            bd->PendingMouseLeaveFrame = 0;
            return true;
        }
        // - In some cases, when detaching a window from main viewport SDL may
        // send SDL_WINDOWEVENT_ENTER one frame too late,
        //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag
        //   operation by clear mouse position. This is why we delay process the
        //   SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for
        //   details.
        // FIXME: Unconfirmed whether this is still needed with SDL3.
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        {
            bd->PendingMouseLeaveFrame = ImGui::GetFrameCount() + 1;
            return true;
        }
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            io.AddFocusEvent(true);
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            io.AddFocusEvent(false);
            return true;
    }
    return false;
}