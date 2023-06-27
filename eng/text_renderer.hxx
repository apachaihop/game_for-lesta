//
// Created by apachai on 24.06.23.
//

#ifndef GAME_TEXT_RENDERER_HXX
#define GAME_TEXT_RENDERER_HXX
#include <string>

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hxx"
#include "texture.hxx"

class text_renderer
{
public:
    texture text_tex;
    std::string message;
    std::string path;
    int font_size;
    glm::vec2 size;
    glm::vec2 position;
    SDL_Color          color;

    text_renderer()
        {};
    text_renderer(shader& s,texture &tex);

    ~text_renderer()
        {};

    void DrawText(std::string mess,
        SDL_Color          color,
                    float  rotate = 0.0f);

    void initRenderData();


private:
    shader       s;
    unsigned int quadVAO;

};

#endif // GAME_TEXT_RENDERER_HXX
