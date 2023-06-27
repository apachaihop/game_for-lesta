//
// Created by apachai on 26.05.23.
//

#ifndef OPENGL_WINDOW_TEXTURE_HXX
#define OPENGL_WINDOW_TEXTURE_HXX
#include <glm/fwd.hpp>
#include <iostream>
#include <string>


#include "glad/glad.h"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <SDL_ttf.h>
class texture
{
    GLuint texID;

public:
    texture()
    {};
    texture(std::string path);
    bool make_text_texture (const std::string &message, const std::string &fontFile,
                             SDL_Color          color, int fontSize);
    GLuint get_ID();
    void   bind();
};

#endif // OPENGL_WINDOW_TEXTURE_HXX
