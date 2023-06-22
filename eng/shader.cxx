//
// Created by apachai on 26.05.23.
//

#include "shader.hxx"
shader::shader(std::string vertexPath, std::string fragmentPath)
{
    std::string vertexCode;
    std::string fragmentCode;

    eng::membuf vertex_mem   = eng::load(vertexPath);
    eng::membuf fragment_mem = eng::load(fragmentPath);

    std::istream vShaderStream(&vertex_mem);

    std::istream fShaderStream(&fragment_mem);

    std::stringstream buff_for_v;
    std::stringstream buff_for_f;
    vShaderStream >> buff_for_v.rdbuf();
    vertexCode = buff_for_v.str();

    fShaderStream >> buff_for_f.rdbuf();
    fragmentCode = buff_for_f.str();

    const char*  vShaderCode = vertexCode.c_str();
    const char*  fShaderCode = fragmentCode.c_str();
    unsigned int vertex, fragment;
    int          success;
    char         infoLog[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    };
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}
void shader::use() const
{
    glUseProgram(ID);
}
void shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
// ------------------------------------------------------------------------
void shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(
        glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(
        glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void shader::setVec4(
    const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}