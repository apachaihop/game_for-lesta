#include "../../opengl_window/eng/engine.hxx"
#include "../../opengl_window/eng/shader.hxx"
#include "../../opengl_window/eng/sprite.hxx"

#ifdef __ANDROID__
#include <SDL3/SDL_main.h>
#define main SDL_main
#endif

#include <cstdlib>
// TODO make button class
// TODO end work with ball class
// TODO make class for loading assets relates the system
// TODO make boxes and collisions with them
// TODO add particle
using namespace eng;

int main(int /*argc*/, char* /*argv*/[])
{

    std::unique_ptr<eng::engine, void (*)(eng::engine*)> engine(
        eng::create_engine(), eng::destroy_engine);

    engine->initialize_engine();

    texture tex_ball("res/ball.png");
    texture tex_hole("res/hole.png");

    shader shader_for_ball("res/vertex.vert", "res/fragment.frag");
    shader shader_for_fone("res/vertex.vert", "res/fragment.frag");
    shader shader_for_holl("res/vertex.vert", "res/fragment.frag");

    sprite holl(shader_for_holl);
    sprite ball(shader_for_ball);
    sprite fone(shader_for_fone);

    glm::mat4 projection = glm::ortho(0.0f,
                                      static_cast<float>(width),
                                      static_cast<float>(height),
                                      0.0f,
                                      -1.0f,
                                      1.0f);
    shader_for_ball.use();
    shader_for_ball.setMat4("projection", projection);
    shader_for_fone.use();
    shader_for_fone.setMat4("projection", projection);
    shader_for_holl.use();
    shader_for_holl.setMat4("projection", projection);

    float angle = 0.0f;
    float dx    = 0.0f;
    float dy    = 0.0f;

    bool continue_loop = true;

    eng::sound_buffer* music = engine->create_sound_buffer("res/sample.wav");
    assert(music != nullptr);

    music->play(eng::sound_buffer::properties::looped);
    while (continue_loop)
    {
        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT)
            {
                continue_loop = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.keysym.sym == SDLK_w)
                {
                    dy += 1.0f;
                    angle = 0.0f;
                }
                if (e.key.keysym.sym == SDLK_s)
                {
                    dy -= 1.0f;
                    angle = 180.0f;
                }
                if (e.key.keysym.sym == SDLK_d)
                {
                    dx += 1.0f;
                    angle = 90.0f;
                }
                if (e.key.keysym.sym == SDLK_a)
                {
                    dx -= 1.0f;
                    angle = -90.0f;
                }
            }
        }

        holl.DrawSprite(tex_hole,
                        glm::vec2(width / 2, 40),
                        glm::vec2(96.0f, 54.0f),
                        angle,
                        glm::vec3(1.0f, 1.0f, 1.0f));
        ball.DrawSprite(tex_ball,
                        glm::vec2(width / 2 + dx, height / 2 - dy),
                        glm::vec2(96.0f, 54.0f),
                        angle,
                        glm::vec3(1.0f, 1.0f, 1.0f));
        engine->swap_buff();
    }

    return EXIT_SUCCESS;
}
