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
private:
    GLuint QuadVAO;
    void init()
    {
        // configure VAO/VBO
        unsigned int VBO;
        float        vertices[] = {
            // pos      // tex
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f
        };

        glGenVertexArrays(1, &this->QuadVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(this->QuadVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

#endif // GAME_GAMEOBJECT_HXX
