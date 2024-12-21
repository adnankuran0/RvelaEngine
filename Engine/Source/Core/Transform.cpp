#include "Transform.h"

Transform::Transform() : position(0.0f), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(1.0f) {}

inline glm::vec3 Transform::getPosition() const { return position; }

inline void Transform::setPosition(const glm::vec3& newPosition) { position = newPosition; }

inline glm::quat Transform::getRotation() const { return rotation; }

inline void Transform::setRotation(const glm::vec3& newRotation) {
    rotation = glm::quat(glm::vec3(glm::radians(newRotation.x),
        glm::radians(newRotation.y), glm::radians(newRotation.z)));
}

inline glm::vec3 Transform::getScale() const { return scale; }

inline void Transform::setScale(const glm::vec3& newScale) { scale = newScale; }

inline glm::mat4 Transform::getMatrix() const {
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

inline glm::vec3 Transform::getForward() const {
    return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

inline glm::vec3 Transform::getUp() const {
    return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

inline glm::vec3 Transform::getRight() const {
    return rotation * glm::vec3(1.0f, 0.0f, 0.0f);
}

inline void Transform::combine(const Transform& other) {
    position += other.position;
    rotation = other.rotation * rotation;
    scale *= other.scale;
}
