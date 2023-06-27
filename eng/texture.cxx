//
// Created by apachai on 26.05.23.
//

#include "texture.hxx"
#include "engine.hxx"
#include <SDL_pixels.h>
#include <SDL_ttf.h>


texture::texture(std::string path)
{
    int            width, height, nrChannels;
    eng::membuf    mem = eng::load(path);
    unsigned char* data =
        stbi_load_from_memory(reinterpret_cast<unsigned char*>(mem.begin()),
                              mem.size(),
                              &width,
                              &height,
                              &nrChannels,
                              0);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load_file texture" << std::endl;
        return;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stbi_image_free(data);
    texID = texture;
    glBindTexture(GL_TEXTURE_2D, 0);
}
GLuint texture::get_ID()
{
    return texID;
}
void texture::bind()
{
    glBindTexture(GL_TEXTURE_2D, texID);
}
bool texture::make_text_texture(const std::string& message,
                                const std::string& fontFile,
                                SDL_Color          color,
                                int                fontSize)
{
    TTF_Init();
    TTF_Font* font = TTF_OpenFont(fontFile.c_str(), fontSize);
    if (font == nullptr)
    {

        return false;
    }

    SDL_Surface* surf = TTF_RenderText_Blended(font,
                                               message.c_str(),
                                               color);
    if (surf == nullptr)
    {
        TTF_CloseFont(font);
        return false;
    }
    surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32);

    SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0 /* xoffset */, 0 /* yoffset */, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    texID = texture;
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}