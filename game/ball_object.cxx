//
// Created by apachai on 21.06.23.
//

#include "ball_object.hxx"

ball_object::ball_object()
        : game_object()
        , radius(12.5f)
{
}

ball_object::ball_object(
        glm::vec2 pos, float radius, glm::vec2 velocity, texture tex, const sprite& spr)
        : game_object(pos,
                      glm::vec2(radius * 2.0f, radius * 2.0f),
                      tex,
                      spr,
                      glm::vec3(1.0f),
                      velocity)
        , radius(radius)
{
}

glm::vec2 ball_object::move(float        dt,
                            unsigned int window_width,
                            unsigned int window_height)
{

    this->position += this->velocity * dt;

    if (this->position.x <= 0.0f)
    {
        this->velocity.x = -this->velocity.x;
        this->position.x = 0.0f;
    }
    else if (this->position.x + this->size.x >= window_width)
    {
        this->velocity.x = -this->velocity.x;
        this->position.x = window_width - this->size.x;
    }
    if (this->position.y <= 0.0f)
    {
        this->velocity.y = -this->velocity.y;
        this->position.y = 0.0f;
    }
    if (this->position.y + this->size.y >= window_height)
    {
        this->velocity.y = -this->velocity.y;
        this->position.y = window_height - this->size.y;
    }
    this->velocity*=0.98;
    return this->position;
}