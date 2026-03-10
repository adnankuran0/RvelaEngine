#include "rvelapch.h"
#include "RigidbodyComponent.h"

using namespace rv;

json RigidbodyComponent::Serialize() const
{
    json j;
    j["bodyType"] = static_cast<int>(bodyType);
    j["autoCalculateMass"] = static_cast<int>(autoCalculateMass);
    j["mass"] = mass;
    j["friction"] = friction;
    j["restitution"] = restitution;
    j["linearDamping"] = linearDamping;
    j["angularDamping"] = angularDamping;
    j["maxLinearVelocity"] = maxLinearVelocity;
    j["maxAngularVelocity"] = maxAngularVelocity;
    j["allowSleep"] = static_cast<int>(allowSleep);

    j["lockRotationX"] = lockRotationX;
    j["lockRotationY"] = lockRotationY;
    j["lockRotationZ"] = lockRotationZ;

    j["lockTranslationX"] = lockTranslationX;
    j["lockTranslationY"] = lockTranslationY;
    j["lockTranslationZ"] = lockTranslationZ;

    j["gravityFactor"] = gravityFactor;
    j["isSensor"] = isSensor;
    j["useCCD"] = useCCD;

    j["collisionLayer"] = collisionFilter.layer;
    j["collisionMask"] = collisionFilter.mask;

    return j;
}

void RigidbodyComponent::Deserialize(const json& j)
{
    bodyType = static_cast<Physics::MotionType>(j.value("bodyType", static_cast<int>(Physics::MotionType::DYNAMIC)));

    autoCalculateMass = static_cast<bool>(j.value("autoCalculateMass", 1.0));
    mass = j.value("mass", 1.0f);
    friction = j.value("friction", 0.5f);
    restitution = j.value("restitution", 0.0f);
    linearDamping = j.value("linearDamping", 0.05f);
    angularDamping = j.value("angularDamping", 0.05f);
    maxLinearVelocity = j.value("maxLinearVelocity", 500.0f);
    maxAngularVelocity = j.value("maxAngularVelocity", 50.0f);
    allowSleep = static_cast<bool>(j.value("allowSleep", 1.0));

    lockRotationX = j.value("lockRotationX", false);
    lockRotationY = j.value("lockRotationY", false);
    lockRotationZ = j.value("lockRotationZ", false);

    lockTranslationX = j.value("lockTranslationX", false);
    lockTranslationY = j.value("lockTranslationY", false);
    lockTranslationZ = j.value("lockTranslationZ", false);

    gravityFactor = j.value("gravityFactor", 1.0f);
    isSensor = j.value("isSensor", false);
    useCCD = j.value("useCCD", false);

    collisionFilter.layer = j.value("collisionLayer", 1u);
    collisionFilter.mask = j.value("collisionMask", 0xFFFFFFFFu);
}