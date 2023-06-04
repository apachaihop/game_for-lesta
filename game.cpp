#include "engine.hxx"
#include "shader.hxx"
#include "sprite.hxx"
#include "texture.hxx"
#include <SDL_events.h>
#include <cstdlib>
#include <memory>

//@ TODO add game class
//@ TODO add camera class
//@ TODO add imgui simple menu
//@ TODO add sound
//@ TODO think about shadows
//@TODO add game obj and heritances from it
using namespace eng;
int main()
{

    std::unique_ptr<eng::engine, void (*)(eng::engine*)> engine(
        eng::create_engine(), eng::destroy_engine);

    engine->initialize_engine();

    texture tex_fone("fone.png");
    texture tex_tank("tank.png");

    eng::vertex   v0 = { 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f, 1.0f };
    eng::vertex   v1 = { 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f };
    eng::vertex   v2 = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    eng::vertex   v3 = { -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f };
    eng::triangle t1(v0, v1, v2);
    eng::triangle t2(v3, v2, v1);

    eng::vertex   v4 = { -0.9f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    eng::vertex   v5 = { -0.9f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
    eng::vertex   v6 = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    eng::vertex   v7 = { -1.0f, -0.9f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f };
    eng::triangle t3(v4, v5, v6);
    eng::triangle t4(v7, v6, v5);

    glm::mat4 transform  = glm::mat4(1.0f);
    glm::mat4 transform0 = glm::mat4(1.0f);
    shader    shader_for_tank("vertex.vert", "fragment.frag");
    shader    shader_for_fone("vertex.vert", "fragment.frag");
    sprite    tank(shader_for_tank);
    sprite    fone(shader_for_fone);
    float     aspect_ratio = width / height;
    glm::mat4 projection   = glm::ortho(0.0f,
                                      static_cast<float>(width),
                                      static_cast<float>(height),
                                      0.0f,
                                      -1.0f,
                                      1.0f);
    shader_for_tank.use();
    shader_for_tank.setMat4("projection", projection);
    shader_for_fone.use();
    shader_for_fone.setMat4("projection", projection);

    float angle = 0.0f;
    float dx    = 0.0f;
    float dy    = 0.0f;
    float cur_x = (t3.v[2].x + (t3.v[1].x - t3.v[2].x) / 2);
    float cur_y = t3.v[2].y + (t3.v[0].y - t3.v[2].y) / 2;
    float new_angle;
    bool  continue_loop = true;
    while (continue_loop)
    {
        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                continue_loop = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.keysym.sym == SDLK_w)
                {
                    dy += 0.01f;
                    angle = 0.0f;
                }
                if (e.key.keysym.sym == SDLK_s)
                {
                    dy -= 0.01f;
                }
                if (e.key.keysym.sym == SDLK_d)
                {
                    dx += 0.01f;
                    angle = -90.0f;
                }
                if (e.key.keysym.sym == SDLK_a)
                {
                    dx -= 0.01f;
                    angle = 90.0f;
                }
            }
        }

        fone.DrawSprite(tex_fone,
                        glm::vec2(0.0f, 0.0f),
                        glm::vec2(width, height),
                        0.0f,
                        glm::vec3(1.0f, 1.0f, 1.0f));

        tank.DrawSprite(tex_tank,
                        glm::vec2(10, height - 40),
                        glm::vec2(40.0f, 40.0f),
                        0.0f,
                        glm::vec3(1.0f, 1.0f, 1.0f));
        engine->swap_buff();
    }

    return EXIT_SUCCESS;
}
