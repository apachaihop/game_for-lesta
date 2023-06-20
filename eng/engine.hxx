#ifndef OPENGL_WINDOW_ENGINE_HXX
#define OPENGL_WINDOW_ENGINE_HXX
#include <SDL_events.h>
#include <iosfwd>
#include <string>

#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace eng
{
constexpr int width  = 1200;
constexpr int height = 920;
enum class event
{
    up,
    left,
    down,
    right,
    button_one,
    button_two,
    select,
    start,
    exit
};

class sound_buffer
{
public:
    enum class properties
    {
        once,
        looped
    };

    virtual ~sound_buffer();
    virtual void play(const properties) = 0;
};
class engine
{
public:
    virtual bool          initialize_engine()                        = 0;
    virtual bool          get_input(event& e)                        = 0;
    virtual bool          rebind_key()                               = 0;
    virtual bool          swap_buff()                                = 0;
    virtual sound_buffer* create_sound_buffer(std::string_view path) = 0;
    virtual void          destroy_sound_buffer(sound_buffer*)        = 0;
};

engine* create_engine();
void    destroy_engine(engine* e);
} // namespace eng
bool ImGui_ImplSDL3_ProcessEvent(SDL_Event* event);
#endif // OPENGL_WINDOW_ENGINE_HXX
