#include "Entity.h"

glm::mat4 Transform::getLocalModelMatrix()
{
    const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
        glm::radians(m_eulerRot.x),
        glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
        glm::radians(m_eulerRot.y),
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
        glm::radians(m_eulerRot.z),
        glm::vec3(0.0f, 0.0f, 1.0f));

    // Y * X * Z
    const glm::mat4 roationMatrix = transformY * transformX * transformZ;

    // translation * rotation * scale (also know as TRS matrix)
    return glm::translate(glm::mat4(1.0f), m_pos) *
        roationMatrix *
        glm::scale(glm::mat4(1.0f), m_scale);
}

void Transform::computeModelMatrix()
{
    m_modelMatrix = getLocalModelMatrix();
    m_isDirty = false;
}

void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
{
    m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
    m_isDirty = false;
}

void Transform::setLocalPosition(const glm::vec3& newPosition)
{
    m_pos = newPosition;
    m_isDirty = true;
}

void Transform::setLocalScale(const glm::vec3& newScale)
{
    m_scale = newScale;
    m_isDirty = true;
}

void Transform::setLocalRotation(const glm::vec3& newRotation)
{
    m_eulerRot = newRotation;
    m_isDirty = true;
}

const glm::vec3& Transform::getLocalPosition()
{
    return m_pos;
}

const glm::mat4& Transform::getModelMatrix()
{
    return m_modelMatrix;
}

bool Transform::isDirty()
{
    return m_isDirty;
}

void Entity::updateSelfAndChild()
{
    if (transform.isDirty())
    {
        forceUpdateSelfAndChild();
        return;
    }

    for (auto&& child : children)
    {
        child->updateSelfAndChild();
    }
}

void Entity::forceUpdateSelfAndChild()
{
    if (parent)
        transform.computeModelMatrix(parent->transform.getModelMatrix());
    else
        transform.computeModelMatrix();

    for (auto&& child : children)
    {
        child->forceUpdateSelfAndChild();
    }
}

void Entity::Draw(Shader& shader)
{
    if (this->transform.isDirty())
        forceUpdateSelfAndChild();
    shader.setMat4("model", transform.getModelMatrix());
    Model::Draw(shader);

    /*for (auto& child : children)
        child->Draw(shader);*/
}
