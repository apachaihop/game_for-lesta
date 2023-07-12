//
// Created by apachai on 11.07.23.
//

#include "wind_object.hxx"
#include <glm/fwd.hpp>
wind_object::wind_object(glm::vec2      pos,
                         glm::vec2      end_pos,
                         Direction direction,
                         float          speed,
                         const sprite&  spr)
{
    this->_msprite       = spr;
    this->position       = pos;
    this->start_position = pos;
    this->end_position   = end_pos;
    this->dir            = direction;
    this->rotation=90.0f;
    this->color=glm::vec3 (1);
    size={128,128};
    wind_speed           = speed;
    _mtex                = texture("res/wind.png");
    switch (dir)
    {
        case up:
            velocity = { 0, -wind_speed };
            break;
        case down:
            velocity = { 0, wind_speed };
            break;
        case right:
            velocity = { wind_speed, 0 };
            break;
        case left:
            velocity = { -wind_speed, 0 };
            break;
    }
    _msprite.initRenderData();
}
glm::vec2 wind_object::move(float dt)
{
    this->position += this->velocity * dt;
    if (glm::length(end_position - position) < 5)
    {
        position = start_position;
    }
}
