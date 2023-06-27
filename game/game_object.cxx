//
// Created by apachai on 21.06.23.
//

#include "game_object.hxx"

game_object::game_object()
    : position(0.0f, 0.0f)
    , size(1.0f, 1.0f)
    , velocity(0.0f)
    , color(1.0f)
    , rotation(0.0f)
    , isSolid(true)
{
}

game_object::game_object(glm::vec2 pos,
                         glm::vec2 size,
                         texture   tex,
                         sprite    spr,
                         glm::vec3 color,
                         glm::vec2 velocity)
    : position(pos)
    , size(size)
    , velocity(velocity)
    , color(color)
    , rotation(0.0f)
    , _mtex(tex)
    , _msprite(spr)
    , isSolid(true)
{
}

void game_object::draw()
{
    _msprite.initRenderData();
    _msprite.DrawSprite(
        _mtex, this->position, this->size, this->rotation, this->color);
}