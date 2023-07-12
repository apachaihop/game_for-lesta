//
// Created by apachai on 11.07.23.
//

#ifndef GAME_WIND_OBJECT_HXX
#define GAME_WIND_OBJECT_HXX
#include "game_object.hxx"
enum Direction
{
    up,
    right,
    down,
    left
};
class wind_object:public game_object
{
public:
    float wind_speed;
    Direction dir;
    glm::vec2 start_position;
    glm::vec2 end_position;
    glm::vec2 move(float        dt);
    wind_object(glm::vec2 position,glm::vec2 end_pos,Direction direction,float speed,const sprite
                                                                                                  &spr);
    void draw()
    {
        _msprite.DrawSprite(
            _mtex, this->position, this->size, this->rotation, this->color);
        _msprite.DrawSprite(
            _mtex, {this->position.x+64,this->position.y+128}, this->size, this->rotation, this->color);
        _msprite.DrawSprite(
            _mtex,{this->position.x-90,this->position.y+64}, this->size, this->rotation, this->color);
    }
};

#endif // GAME_WIND_OBJECT_HXX
