//
// Created by apachai on 21.06.23.
//

#include "game_object.hxx"

game_object::game_object()
    : position(0.0f, 0.0f)
    , size(1.0f, 1.0f)
    , velocity(0.0f)
    , color(1.0f)
    , rotation(0.0f)
    , isSolid(true)
{

}

game_object::game_object(glm::vec2 pos,
                         glm::vec2 size,
                         texture   tex,
                         sprite    spr,
                         glm::vec3 color,
                         glm::vec2 velocity)
    : position(pos)
    , size(size)
    , velocity(velocity)
    , color(color)
    , rotation(0.0f)
    , _mtex(tex)
    , _msprite(spr)
    , isSolid(true)
{
    init();
}

void game_object::draw()
{
    _msprite.s.use();
    glm::mat4 model = glm::mat4(1.0f);
    model           = glm::translate(
        model,
        glm::vec3(position,
                  0.0f)); // first translate (transformations are: scale happens
                          // first, then rotation, and then final translation
                          // happens; reversed order)
    model=glm::translate(model,glm::vec3(0,size.y/2,0));

    model = glm::rotate(model,
                        glm::radians(rotation),
                        glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate

    model=glm::translate(model,glm::vec3(0,-size.y/2,0));
    model = glm::scale(model, glm::vec3(size.x,size.y, 1.0f));     // last scale

    _msprite.s.setMat4("model", model);

    _msprite.s.setVec3("spriteColor", color.x, color.y, color.z);

    _mtex.bind();

    glActiveTexture(GL_TEXTURE0);

    _msprite.s.setInt("image", 0);

    glBindVertexArray(this->QuadVAO);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

}