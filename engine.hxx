#ifndef OPENGL_WINDOW_ENGINE_HXX
#define OPENGL_WINDOW_ENGINE_HXX
#include <iosfwd>
#include <string>

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

struct vertex
{
    float x = 0.f; // vertex position
    float y = 0.f;
    float z = 0.f;

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;

    float tx = 0.f; // texture coordinate
    float ty = 0.f;
};

struct triangle
{
    triangle()
    {
        v[0] = vertex();
        v[1] = vertex();
        v[2] = vertex();
    }
    triangle(vertex v0, vertex v1, vertex v2)
    {
        v[0] = v0;
        v[1] = v1;
        v[2] = v2;
    }
    vertex v[3];
};

class engine
{
public:
    virtual bool initialize_engine()                     = 0;
    virtual bool get_input(event& e)                     = 0;
    virtual bool rebind_key()                            = 0;
    virtual void draw_triangle(triangle t1, triangle t2) = 0;
    virtual bool swap_buff()                             = 0;
    virtual int  load_texture(std::string path)          = 0;
    virtual bool draw_texture(triangle  t1,
                              triangle  t2,
                              int       texHandle,
                              glm::mat4 transform)       = 0;
};

engine* create_engine();
void    destroy_engine(engine* e);
} // namespace eng
#endif // OPENGL_WINDOW_ENGINE_HXX
