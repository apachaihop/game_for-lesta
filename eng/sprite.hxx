//
// Created by apachai on 29.05.23.
//

#ifndef OPENGL_WINDOW_SPRITE_HXX
#define OPENGL_WINDOW_SPRITE_HXX

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hxx"
#include "texture.hxx"

class sprite
{
public:
    sprite(shader& s);

    ~sprite();

    void DrawSprite(texture&  texture,
                    glm::vec2 position,
                    glm::vec2 size   = glm::vec2(10.0f, 10.0f),
                    float     rotate = 0.0f,
                    glm::vec3 color  = glm::vec3(1.0f));

private:
    shader       s;
    unsigned int quadVAO;

    void initRenderData();
};

#endif // OPENGL_WINDOW_SPRITE_HXX
