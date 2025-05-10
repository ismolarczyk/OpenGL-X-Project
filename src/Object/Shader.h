#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, const char* computePath = nullptr);
    
    Shader(const char* computePath);

    // activate the shader
    // ------------------------------------------------------------------------
    void use();
    
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const;
    
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const;
    
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const;
    
    void setMat4(const std::string& name, const glm::mat4& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec2(const std::string& name, const glm::vec2 value);
private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type);
    
};
#endif
