//
// Created by apachai on 21.06.23.
//

#include "game_object.hxx"

game_object::game_object()
    : Position(0.0f, 0.0f)
    , Size(1.0f, 1.0f)
    , Velocity(0.0f)
    , Color(1.0f)
    , Rotation(0.0f)
    , IsSolid(false)
{
}

game_object::game_object(glm::vec2 pos,
                         glm::vec2 size,
                         texture   tex,
                         sprite    spr,
                         glm::vec3 color,
                         glm::vec2 velocity)
    : Position(pos)
    , Size(size)
    , Velocity(velocity)
    , Color(color)
    , Rotation(0.0f)
    , _mtex(tex)
    , _msprite(spr)
    , IsSolid(false)
{
}

void game_object::Draw()
{
    _msprite.DrawSprite(
        _mtex, this->Position, this->Size, this->Rotation, this->Color);
}