#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp> //glm::mat4
#include <list> //std::list
#include <array> //std::array
#include <memory> //std::unique_ptr
#include "Scene/Camera.h"
#include "Model.h"
#include <glm/gtc/matrix_transform.hpp>

class Transform
{
protected:
    //Local space information
    glm::vec3 m_pos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_eulerRot = { 0.0f, 0.0f, 0.0f }; //In degrees
    glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

    //Global space information concatenate in matrix
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);

    //Dirty flag
    bool m_isDirty = true;

protected:
    glm::mat4 getLocalModelMatrix();
public:

    void computeModelMatrix();

    void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);

    void setLocalPosition(const glm::vec3& newPosition);

    void setLocalScale(const glm::vec3& newScale);
    
    void setLocalRotation(const glm::vec3& newRotation);


    const glm::vec3& getLocalPosition();


    const glm::mat4& getModelMatrix();

    bool isDirty();
};

class Entity : public Model
{
public:
    //Scene graph
    std::list<std::unique_ptr<Entity>> children;
    Entity* parent = nullptr;

    //Space information
    Transform transform;

    // constructor, expects a filepath to a 3D model.
    Entity(string const& path, bool gamma = false) : Model(path, gamma), transform()
    {}

    //Add child. Argument input is argument of any constructor that you create.
    //By default you can use the default constructor and don't put argument input.
    template<typename... TArgs>
    void addChild(const TArgs&... args)
    {
        children.emplace_back(std::make_unique<Entity>(args...));
        children.back()->parent = this;
    }

    //Update transform if it was changed
    void updateSelfAndChild();

    //Force update of transform even if local space don't change
    void forceUpdateSelfAndChild();
    void Draw(Shader& shader) override;
};
#endif
