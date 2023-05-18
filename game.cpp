#include "engine.hxx"
#include <SDL_events.h>
#include <cstdlib>
#include <fstream>
#include <memory>

using namespace eng;
int main()
{

    std::unique_ptr<eng::engine, void (*)(eng::engine*)> engine(
        eng::create_engine(), eng::destroy_engine);

    engine->initialize_engine();

    int tex_fone = engine->load_texture("fone.png");
    int tex_tank = engine->load_texture("tank.png");

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

    float aspect_ratio = width / height;

    transform  = glm::scale(transform, glm::vec3(1.0f, aspect_ratio, 1.0f));
    transform0 = glm::scale(transform0, glm::vec3(1.0f, aspect_ratio, 1.0f));

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
                    angle = -180.0f;
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
        transform = glm::translate(transform, glm::vec3(cur_x, cur_y, 0.0f));
        transform = glm::translate(transform, glm::vec3(dx, dy, 0.0f));

        transform = glm::rotate(
            transform, glm::radians(angle), glm::vec3(0.0, 0.0, 1.0));

        transform = glm::translate(transform, glm::vec3(-cur_x, -cur_y, 0.0f));

        engine->draw_texture(t1, t2, tex_fone, transform0);
        engine->draw_texture(t3, t4, tex_tank, transform);
        engine->swap_buff();
        dx    = 0.0f;
        dy    = 0.0f;
        angle = 0;
    }

    return EXIT_SUCCESS;
}
