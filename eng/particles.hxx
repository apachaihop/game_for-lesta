//
// Created by apachai on 27.06.23.
//

#ifndef GAME_PARTICLES_HXX
#define GAME_PARTICLES_HXX
#include <vector>

#include "glad/glad.h"
#include <glm/glm.hpp>

#include "shader.hxx"
#include "texture.hxx"
#include "../game/game_object.hxx"

struct Particle {
    glm::vec2 Position, Velocity;
    glm::vec4 Color;
    float     Life;

    Particle() : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) { }
};



class ParticleGenerator
{
public:

    ParticleGenerator(shader s,texture tex, unsigned int amount);

    void Update(float dt, game_object &object, unsigned int newParticles, glm::vec2 offset = glm::vec2(0.0f, 0.0f));

    void Draw();
private:
    // state
    std::vector<Particle> particles;
    unsigned int amount;
    // render state
    shader _mshader;
    texture _mtexture;
    unsigned int VAO;

    void init();

    unsigned int firstUnusedParticle();
    // respawns particle
    void respawnParticle(Particle &particle, game_object &object, glm::vec2 offset = glm::vec2(0.0f, 0.0f));
};

#endif // GAME_PARTICLES_HXX
