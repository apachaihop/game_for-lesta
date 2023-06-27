#ifndef OPENGL_WINDOW_ENGINE_HXX
#define OPENGL_WINDOW_ENGINE_HXX
#include <SDL_events.h>
#include <iosfwd>
#include <memory>
#include <streambuf>
#include <string>

#include "../game/ball_object.hxx"
#include "../game/game_object.hxx"
#include "text_renderer.hxx"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace eng
{
constexpr int  width    = 1280;
constexpr int  height   = 640;
constexpr bool dev_mode = false;
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
struct membuf : public std::streambuf
{
    membuf()
        : std::streambuf()
        , buf()
        , buf_size(0)
    {
    }
    membuf(std::unique_ptr<char[]> buffer, size_t size)
        : std::streambuf()
        , buf(std::move(buffer))
        , buf_size(size)
    {
        char* beg_ptr = buf.get();
        char* end_ptr = beg_ptr + buf_size;
        setg(beg_ptr, beg_ptr, end_ptr);
        setp(beg_ptr, end_ptr);
    }
    membuf(membuf&& other)
    {
        setp(nullptr, nullptr);
        setg(nullptr, nullptr, nullptr);

        other.swap(*this);

        buf      = std::move(other.buf);
        buf_size = other.buf_size;

        other.buf_size = 0;
    }

    pos_type seekoff(off_type               pos,
                     std::ios_base::seekdir seek_dir,
                     std::ios_base::openmode) override
    {

        if (seek_dir == std::ios_base::beg)
        {
            return 0 + pos;
        }
        else if (seek_dir == std::ios_base::end)
        {
            return buf_size + pos;
        }
        else
        {
            return egptr() - gptr();
        }
    }

    char*  begin() const { return eback(); }
    size_t size() const { return buf_size; }

private:
    std::unique_ptr<char[]> buf;
    size_t                  buf_size;
};

membuf load(std::string path);

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
enum Direction
{
    UP,
    RIGHT,
    DOWN,
    LEFT
};

typedef std::tuple<bool, Direction, glm::vec2> Collision;

class engine
{
public:
    int               width_a;
    int               height_a;
    int               mouse_X, mouse_Y;
    bool              mouse_pressed       = false, mouse_realesed;
    virtual bool      initialize_engine() = 0;
    virtual bool      get_input(event& e) = 0;
    virtual bool      rebind_key()        = 0;
    virtual bool      swap_buff()         = 0;
    virtual Collision check_collision(ball_object& ball, game_object& obj) = 0;
    virtual Collision check_collision(ball_object& ball)                   = 0;
    virtual Collision check_collision(text_renderer& text)                   = 0;
    virtual sound_buffer* create_sound_buffer(std::string_view path)       = 0;
    virtual void          destroy_sound_buffer(sound_buffer*)              = 0;
};

engine* create_engine();
void    destroy_engine(engine* e);
} // namespace eng
bool ImGui_ImplSDL3_ProcessEvent(SDL_Event* event);
#endif // OPENGL_WINDOW_ENGINE_HXX
