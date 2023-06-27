//
// Created by apachai on 29.05.23.
//

#include "sprite.hxx"
#include "engine.hxx"
#include "shader.hxx"
#define OM_GL_CHECK()                                                          \
    {                                                                          \
        const int err = static_cast<int>(glGetError());                        \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            switch (err)                                                       \
            {                                                                  \
                case GL_INVALID_ENUM:                                          \
                    std::cerr << "GL_INVALID_ENUM" << std::endl;               \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    std::cerr << "GL_INVALID_VALUE" << std::endl;              \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    std::cerr << "GL_INVALID_OPERATION" << std::endl;          \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"            \
                              << std::endl;                                    \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    std::cerr << "GL_OUT_OF_MEMORY" << std::endl;              \
                    break;                                                     \
            }                                                                  \
            assert(false);                                                     \
        }                                                                      \
    }

sprite::sprite(class shader& s)
{
    this->s = s;
    this->initRenderData();
}

sprite::~sprite()
{
    glDeleteVertexArrays(1, &this->quadVAO);
}

void sprite::DrawSprite(texture&  texture,
                        glm::vec2 position,
                        glm::vec2 size,
                        float     rotate,
                        glm::vec3 color)
{
    // prepare transformations
    this->s.use();
    glm::mat4 model = glm::mat4(1.0f);
    model           = glm::translate(
        model,
        glm::vec3(position,
                  0.0f)); // first translate (transformations are: scale happens
                          // first, then rotation, and then final translation
                          // happens; reversed order)
    model=glm::translate(model,glm::vec3(0,size.y/2,0));

    model = glm::rotate(model,
                        glm::radians(rotate),
                        glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate

    model=glm::translate(model,glm::vec3(0,-size.y/2,0));
    model = glm::scale(model, glm::vec3(size, 1.0f));     // last scale

    this->s.setMat4("model", model);
    OM_GL_CHECK()
    // render textured quad
    this->s.setVec3("spriteColor", color.x, color.y, color.z);
    OM_GL_CHECK()
    texture.bind();

    glActiveTexture(GL_TEXTURE0);
    OM_GL_CHECK()
    s.setInt("image", 0);
    OM_GL_CHECK()
    glBindVertexArray(this->quadVAO);
    OM_GL_CHECK()
    glDrawArrays(GL_TRIANGLES, 0, 6);
    OM_GL_CHECK()
    glBindVertexArray(0);
    OM_GL_CHECK()
}

void sprite::initRenderData()
{
    // configure VAO/VBO
    unsigned int VBO;
    float        vertices[] = {
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &this->quadVAO);
    glGenBuffers(1, &VBO);
    OM_GL_CHECK()
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    OM_GL_CHECK()
    glBindVertexArray(this->quadVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    OM_GL_CHECK()
    glBindVertexArray(0);
}