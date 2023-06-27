//
// Created by apachai on 21.06.23.
//

#ifndef GAME_GAMEOBJECT_HXX
#define GAME_GAMEOBJECT_HXX

#include <../glad/glad.h>
#include <glm/glm.hpp>

#include "../eng/sprite.hxx"
#include "../eng/texture.hxx"

class game_object
{
public:
    glm::vec2 position, size, velocity;
    glm::vec3 color;
    float     rotation;
    bool      isSolid=true;

    texture _mtex;
    sprite  _msprite;

    game_object();
    game_object(glm::vec2 pos,
                glm::vec2 size,
                texture   tex,
                sprite    spr,
                glm::vec3 color    = glm::vec3(1.0f),
                glm::vec2 velocity = glm::vec2(0.0f, 0.0f));
    // draw sprite
    virtual void draw();
};

#endif // GAME_GAMEOBJECT_HXX
