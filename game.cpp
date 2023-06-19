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

    texture tex_fone("../fone.png");
    texture tex_tank("../tank.png");

    glm::mat4 transform  = glm::mat4(1.0f);
    glm::mat4 transform0 = glm::mat4(1.0f);
    shader    shader_for_tank("../vertex.vert", "../fragment.frag");
    shader    shader_for_fone("../vertex.vert", "../fragment.frag");
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

    float new_angle;
    bool  continue_loop = true;

    eng::sound_buffer* music = engine->create_sound_buffer("../sample.wav");
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

        fone.DrawSprite(tex_fone,
                        glm::vec2(0.0f, 0.0f),
                        glm::vec2(width, height),
                        0.0f,
                        glm::vec3(1.0f, 1.0f, 1.0f));

        tank.DrawSprite(tex_tank,
                        glm::vec2(10 + dx, height - 40 - dy),
                        glm::vec2(40.0f, 40.0f),
                        angle,
                        glm::vec3(1.0f, 1.0f, 1.0f));
        engine->swap_buff();
    }

    return EXIT_SUCCESS;
}
