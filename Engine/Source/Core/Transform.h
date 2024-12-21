#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

class Transform {
private:
    glm::vec3 position;  
    glm::quat rotation;  
    glm::vec3 scale;     

public:
    Transform();
        

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& newPosition);

    glm::quat getRotation() const;
    void setRotation(const glm::vec3& newRotation);

    glm::vec3 getScale() const;
    void setScale(const glm::vec3& newScale);

    glm::mat4 getMatrix() const;

    glm::vec3 getForward() const;

    glm::vec3 getUp() const;

    glm::vec3 getRight() const;

    void combine(const Transform& other);
};


