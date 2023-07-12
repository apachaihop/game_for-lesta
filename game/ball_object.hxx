//
// Created by apachai on 21.06.23.
//

#ifndef GAME_BALL_OBJECT_HXX
#define GAME_BALL_OBJECT_HXX

#include <../glad/glad.h>
#include <glm/glm.hpp>

#include "game_object.hxx"

class ball_object : public game_object
{
public:
    float radius;

    ball_object();

    ball_object(glm::vec2 pos,
                float     radius,
                glm::vec2 velocity,
                texture   tex,
                const sprite&    spr);

    glm::vec2 move(float        dt,
                   unsigned int window_width,
                   unsigned int window_height);
    void draw()
    {

        size=glm::vec2(radius * 2.0f, radius * 2.0f);
        _msprite.DrawSprite(
            _mtex, this->position, this->size, this->rotation, this->color);
    }
};

#endif // GAME_BALL_OBJECT_HXX
