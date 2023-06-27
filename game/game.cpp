#include "../../game_for-lesta/eng/engine.hxx"
#include "../eng/particles.hxx"
#include "../eng/text_renderer.hxx"

#include "ball_object.hxx"
#include "game_object.hxx"
#include "imgui.h"

#include <SDL_events.h>
#include <SDL_pixels.h>
#include <ctime>
#include <glm/geometric.hpp>
#include <math.h>
#include <time.h>
#include <vector>

#ifdef __ANDROID__
#include <SDL3/SDL_main.h>
#define main SDL_main
#endif

#include <algorithm>
#include <chrono>
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
    texture tex_box("res/box.png");
    texture tex_arrow("res/arrow.png");
    texture tex_fire_particle("res/fire.png");
    texture text;

    shader shader_for_ball("res/vertex.vert", "res/fragment.frag");
    shader shader_for_holl("res/vertex.vert", "res/fragment.frag");
    shader shader_for_particle("res/particle.vert", "res/particle.frag");

    text_renderer t_render(shader_for_ball, text);
    t_render.message   = "Start";
    t_render.path      = "res/verdana.ttf";
    t_render.font_size = 32;
    t_render.position  = (glm::vec2(engine->width_a / 2, engine->height_a / 2));
    t_render.size      = glm::vec2(50, 32);
    SDL_Color col;
    col.r = 0;
    col.b = 0;
    col.g = 0;

    sprite            holl(shader_for_holl);
    sprite            ball(shader_for_ball);
    sprite            box(shader_for_ball);
    sprite            s_arrow(shader_for_ball);
    sprite            fire(shader_for_ball);
    ParticleGenerator particles(shader_for_particle, tex_fire_particle, 100);
    glm::mat4         projection = glm::ortho(0.0f,
                                      static_cast<float>(engine->width_a),
                                      static_cast<float>(engine->height_a),
                                      0.0f,
                                      -1.0f,
                                      1.0f);
    shader_for_ball.use();
    shader_for_ball.setMat4("projection", projection);
    shader_for_holl.use();
    shader_for_holl.setMat4("projection", projection);
    shader_for_particle.use();
    shader_for_particle.setMat4("projection", projection);

    ball_object obj_ball(glm::vec2(engine->width_a / 2, engine->height_a / 2),
                         15.0f,
                         glm::vec2(0.0f, 0.0f),
                         tex_ball,
                         ball);

    game_object obj_holl(glm::vec2(825, 40), { 64, 64 }, tex_hole, holl);
    obj_holl.isSolid = true;

    std::vector<game_object> boxes;
    boxes.push_back({ glm::vec2(740, 0), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(740, 64), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(740, 128), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(804, 128), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(868, 128), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(932, 128), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(932, 64), glm::vec2(70, 70), tex_box, box });
    boxes.push_back({ glm::vec2(932, 0), glm::vec2(70, 70), tex_box, box });

    for (auto box : boxes)
    {
        box.isSolid = true;
    }
    boxes.back().isSolid = false;

    glm::vec2 vel;

    bool continue_loop = true;
    bool start_game    = false;
    bool win=false;

    eng::sound_buffer* music = engine->create_sound_buffer("res/sample.wav");
    assert(music != nullptr);
    eng::sound_buffer* strike =
        engine->create_sound_buffer("res/otskok-myacha.wav");
    assert(music != nullptr);

    music->play(eng::sound_buffer::properties::looped);

    using clock_timer = std::chrono::high_resolution_clock;
    using nano_sec    = std::chrono::nanoseconds;
    using milli_sec   = std::chrono::milliseconds;
    using sec         = std::chrono::seconds;
    using time_point  = std::chrono::time_point<clock_timer, nano_sec>;

    clock_timer timer;

    time_point start = timer.now();

    glm::vec2 diff;
    glm::vec2 center;
    while (continue_loop)
    {
        SDL_Event  e;
        time_point end_last_frame = timer.now();
        //
        //
        //
        //
        // Update
        //
        //
        //
        //
        //
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT)
            {
                continue_loop = false;
                break;
            }
            if (e.type == SDL_EVENT_FINGER_MOTION)
            {

                engine->mouse_X = ImGui::GetMousePos().x;
                engine->mouse_Y = ImGui::GetMousePos().y;
                if (engine->mouse_pressed)
                {
                    glm::vec2 mouse(engine->mouse_X, engine->mouse_Y);
                    center = obj_ball.position + obj_ball.radius;
                    diff   = center - mouse;

                    float initianal_vel_x =
                        cos(atan2(diff.y, diff.x)) *
                        std::clamp(glm::length(diff), 0.0f, 120.0f) / 120;

                    vel.x = initianal_vel_x * 2;

                    std::cout << vel.x << " ";

                    float initianal_vel_y =
                        sin(atan2(diff.y, diff.x)) *
                        std::clamp(glm::length(diff), 0.0f, 120.0f) / 120;

                    vel.y = initianal_vel_y * 2;

                    std::cout << vel.y << " " << std::endl;

                    obj_ball.velocity = vel;
                    std::cout << glm::length(vel) << std::endl;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {

                engine->mouse_X = e.motion.x;
                engine->mouse_Y = e.motion.y;
                if (engine->mouse_pressed)
                {
                    glm::vec2 mouse(engine->mouse_X, engine->mouse_Y);
                    center = obj_ball.position + obj_ball.radius;
                    diff   = center - mouse;

                    float initianal_vel_x =
                        cos(atan2(diff.y, diff.x)) *
                        std::clamp(glm::length(diff), 0.0f, 120.0f) / 120;

                    vel.x = initianal_vel_x * 2;

                    std::cout << vel.x << " ";

                    float initianal_vel_y =
                        sin(atan2(diff.y, diff.x)) *
                        std::clamp(glm::length(diff), 0.0f, 120.0f) / 120;

                    vel.y = initianal_vel_y * 2;

                    std::cout << vel.y << " " << std::endl;

                    obj_ball.velocity = vel;
                    std::cout << glm::length(vel) << std::endl;
                }
            }
            if (e.type == SDL_EVENT_FINGER_DOWN)
            {
                engine->mouse_X = ImGui::GetMousePos().x;
                engine->mouse_Y = ImGui::GetMousePos().y;
                Collision coll  = engine->check_collision(obj_ball);
                if (std::get<0>(coll) == true)
                {
                    engine->mouse_pressed  = true;
                    engine->mouse_realesed = false;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if(start_game)
                {
                    Collision coll = engine->check_collision(obj_ball);
                    if (e.button.button == SDL_BUTTON_LEFT)
                    {
                        if (std::get<0>(coll) == true)
                        {
                            engine->mouse_pressed  = true;
                            engine->mouse_realesed = false;
                        }
                    }
                }
                else
                {
                    Collision coll = engine->check_collision(t_render);
                    if (e.button.button == SDL_BUTTON_LEFT)
                    {
                        if (std::get<0>(coll) == true)
                        {
                            start_game=true;
                            engine->mouse_pressed  = true;
                            engine->mouse_realesed = false;
                        }
                    }
                }
            }
            if (e.type == SDL_EVENT_FINGER_UP)
            {
                engine->mouse_X       = ImGui::GetMousePos().x;
                engine->mouse_Y       = ImGui::GetMousePos().y;
                engine->mouse_pressed = false;

                engine->mouse_realesed = true;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    engine->mouse_pressed = false;

                    engine->mouse_realesed = true;
                }
            }
        }

        milli_sec frame_delta =
            std::chrono::duration_cast<milli_sec>(end_last_frame - start);

        Collision coll = engine->check_collision(obj_ball, obj_holl);
        if (std::get<0>(coll) == true)
        {
            obj_ball.velocity.x = 0;
            obj_ball.velocity.y = 0;
            obj_ball.move(0, 0, 0);
            obj_ball.position =
                obj_holl.position +
                glm::vec2(obj_holl.size.x / 2, obj_holl.size.y / 2) -
                obj_ball.radius;
            clock_t start = clock();
            while (obj_ball.radius >= 1)
            {
                if ((clock() - start) >= 1500)
                {
                    obj_ball.radius -= 0.2;
                    obj_holl.draw();
                    obj_ball.draw();
                    obj_ball.position =
                        obj_holl.position +
                        glm::vec2(obj_holl.size.x / 2, obj_holl.size.y / 2) -
                        obj_ball.radius;
                    for (auto box : boxes)
                    {
                        box.draw();
                    }
                    engine->swap_buff();
                    start = clock();
                }
            }
            obj_ball.radius = 0;
            t_render.DrawText("WIN!!", col);
            win=true;
        }

        for (auto box : boxes)
        {
            Collision coll = engine->check_collision(obj_ball, box);
            if (std::get<0>(coll) == true && box.isSolid == true)
            {
                strike->play(eng::sound_buffer::properties::once);
                if (std::get<1>(coll) == eng::LEFT ||
                    std::get<1>(coll) == eng::RIGHT)
                {
                    obj_ball.velocity.x =
                        -obj_ball.velocity.x; // reverse horizontal velocity
                    // relocate
                    float penetration =
                        obj_ball.radius - std::abs(std::get<2>(coll).x);
                    if (std::get<1>(coll) == LEFT)
                        obj_ball.position.x +=
                            penetration; // move ball to right
                    else
                        obj_ball.position.x -=
                            penetration; // move ball to left;
                }
                else                     // vertical collision
                {
                    obj_ball.velocity.y =
                        -obj_ball.velocity.y; // reverse horizontal velocity
                    // relocate
                    float penetration =
                        obj_ball.radius - std::abs(std::get<2>(coll).y);
                    if (std::get<1>(coll) == UP)
                        obj_ball.position.y -=
                            penetration; // move ball to right
                    else
                        obj_ball.position.y +=
                            penetration; // move ball to left;
                }
            }
        }
        if (engine->mouse_realesed)
        {
            obj_ball.move(
                frame_delta.count(), engine->width_a, engine->height_a);
        }

        //
        //
        //
        //
        // Render
        //
        //
        //
        //

        if (start_game)
        {
            obj_holl.draw();

            for (auto box : boxes)
            {
                box.draw();
            }
            if (engine->mouse_pressed)
            {

                s_arrow.DrawSprite(
                    tex_arrow,
                    obj_ball.position + obj_ball.radius - 5.0F,
                    glm::vec2(std::clamp(glm::length(diff), 0.0f, 120.0f), 42),
                    atan2(diff.y, diff.x) * 180 / 3.14,
                    glm::vec3(
                        255 * std::clamp(glm::length(diff), 0.0f, 120.0f) / 120,
                        255 - 255 *
                                  std::clamp(glm::length(diff), 0.0f, 120.0f) /
                                  120,
                        0.0f));
            }
            if (engine->mouse_realesed &&
                std::clamp(glm::length(diff), 0.0f, 120.0f) == 120 &&
                glm::length(obj_ball.velocity) >= 0.5)
            {
                particles.Update(
                    0.01, obj_ball, 40, glm::vec2(obj_ball.radius));
                particles.Update(
                    0.01, obj_ball, 20, glm::vec2(obj_ball.radius + 5));
                particles.Update(
                    0.01, obj_ball, 20, glm::vec2(obj_ball.radius - 5));
                particles.Update(
                    0.01, obj_ball, 10, glm::vec2(obj_ball.radius + 10));
                particles.Update(
                    0.01, obj_ball, 10, glm::vec2(obj_ball.radius - 10));
                particles.Draw();
            }
            particles.Update(0.01, obj_ball, 40, glm::vec2(obj_ball.radius));
            particles.Update(
                0.01, obj_ball, 20, glm::vec2(obj_ball.radius + 5));
            particles.Update(
                0.01, obj_ball, 20, glm::vec2(obj_ball.radius - 5));
            particles.Update(
                0.01, obj_ball, 10, glm::vec2(obj_ball.radius + 10));
            particles.Update(
                0.01, obj_ball, 10, glm::vec2(obj_ball.radius - 10));
            obj_ball.draw();
            if (engine->mouse_realesed &&
                std::clamp(glm::length(diff), 0.0f, 120.0f) == 120 &&
                glm::length(obj_ball.velocity) >= 0.5)
            {

                particles.Update(
                    0.01, obj_ball, 10, glm::vec2(obj_ball.radius + 10));
                particles.Update(
                    0.01, obj_ball, 10, glm::vec2(obj_ball.radius - 10));
                particles.Draw();
            }
        }
        else
        {
            t_render.DrawText("Start", col);
        }
        if(win)
        {
            t_render.DrawText("WIN!!", col);
        }
        engine->swap_buff();
        start = end_last_frame;
    }

    return EXIT_SUCCESS;
}
