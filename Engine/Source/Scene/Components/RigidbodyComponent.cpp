#include "rvelapch.h"
#include "RigidbodyComponent.h"

using namespace rv;

json RigidbodyComponent::Serialize() const
{
    json j;
    j["bodyType"] = static_cast<int>(bodyType);
    j["mass"] = mass;
    j["friction"] = friction;
    j["restitution"] = restitution;
    j["linearDamping"] = linearDamping;
    j["angularDamping"] = angularDamping;

    j["lockRotationX"] = lockRotationX;
    j["lockRotationY"] = lockRotationY;
    j["lockRotationZ"] = lockRotationZ;

    j["lockTranslationX"] = lockTranslationX;
    j["lockTranslationY"] = lockTranslationY;
    j["lockTranslationZ"] = lockTranslationZ;

    j["gravityFactor"] = gravityFactor;
    j["isSensor"] = isSensor;
    j["useCCD"] = useCCD;

    return j;
}

void RigidbodyComponent::Deserialize(const json& j)
{
    bodyType = static_cast<Physics::BodyType>(j.value("bodyType", static_cast<int>(Physics::BodyType::DYNAMIC)));

    mass = j.value("mass", 1.0f);
    friction = j.value("friction", 0.5f);
    restitution = j.value("restitution", 0.2f);
    linearDamping = j.value("linearDamping", 0.05f);
    angularDamping = j.value("angularDamping", 0.05f);

    lockRotationX = j.value("lockRotationX", false);
    lockRotationY = j.value("lockRotationY", false);
    lockRotationZ = j.value("lockRotationZ", false);

    lockTranslationX = j.value("lockTranslationX", false);
    lockTranslationY = j.value("lockTranslationY", false);
    lockTranslationZ = j.value("lockTranslationZ", false);

    gravityFactor = j.value("gravityFactor", 1.0f);
    isSensor = j.value("isSensor", false);
    useCCD = j.value("useCCD", false);
}