//
// Created by apachai on 24.06.23.
//

#include "text_renderer.hxx"

text_renderer::text_renderer(shader& s, texture& tex)
{
    this->s = s;
  this->initRenderData();
}

void text_renderer::DrawText( std::string mess,
                             SDL_Color          color,
                             float rotate)
{

  message=mess;

this->text_tex.make_text_texture(mess,this->path,color,this->font_size);

this->s.use();
glm::mat4 model = glm::mat4(1.0f);
model           = glm::translate(
    model,
    glm::vec3(position,
              0.0f)); // first translate (transformations are: scale happens
                                // first, then rotation, and then final translation
                                // happens; reversed order)

model = glm::translate(
    model,
    glm::vec3(0.5f * size.x,
              0.5f * size.y,
              0.0f)); // move origin of rotation to center of quad
model = glm::rotate(model,
                    glm::radians(rotate),
                    glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate
model = glm::translate(
    model,
    glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // move origin back

model = glm::scale(model, glm::vec3(size, 1.0f));     // last scale

this->s.setMat4("model", model);

// render textured quad
this->s.setVec3("spriteColor", color.r, color.g, color.b);

text_tex.bind();

glActiveTexture(GL_TEXTURE0);

s.setInt("image", 0);

glBindVertexArray(this->quadVAO);

glDrawArrays(GL_TRIANGLES, 0, 6);

glBindVertexArray(0);
glDeleteTextures (1, text_tex.get_ID());


}
void text_renderer::initRenderData()
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

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

glBindVertexArray(this->quadVAO);
glEnableVertexAttribArray(0);
glVertexAttribPointer(
    0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindVertexArray(0);
}