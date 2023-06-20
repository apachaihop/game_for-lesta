//
// Created by apachai on 26.05.23.
//

#ifndef OPENGL_WINDOW_TEXTURE_HXX
#define OPENGL_WINDOW_TEXTURE_HXX
#include <iostream>
#include <string>

#include "glad/glad.h"
#include "stb_image.h"

class texture
{
    GLuint texID;

public:
    texture(std::string path);
    GLuint get_ID();
    void   bind();
};

#endif // OPENGL_WINDOW_TEXTURE_HXX
