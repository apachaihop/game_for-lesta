//
// Created by apachai on 26.05.23.
//

#ifndef OPENGL_WINDOW_SHADER_HXX
#define OPENGL_WINDOW_SHADER_HXX
#include "glad/glad.h"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>
class shader
{
    GLuint ID;

public:
    shader(){};
    shader(std::string vertexPath, std::string fragmentPath);

    void use() const;

    void setInt(const std::string& name, int value) const;

    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const;

    void setMat3(const std::string& name, const glm::mat3& mat) const;

    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    void setVec3(const std::string& name, float x, float y, float z) const;

    void setVec4(
        const std::string& name, float x, float y, float z, float w) const;
};

#endif // OPENGL_WINDOW_SHADER_HXX
