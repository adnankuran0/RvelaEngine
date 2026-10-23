#include "rvelapch.h"
#include "PhysicsConstraintComponent.h"

namespace rv {

json PhysicsConstraintComponent::Serialize() const
{
	json j;
	j["connectedEntity"] = connectedEntityUUID;
	j["type"] = static_cast<int>(type);
	j["space"] = static_cast<int>(space);
	j["enabled"] = enabled;
	j["autoDetectPoints"] = autoDetectPoints;

	j["anchorA"] = { anchorA.x, anchorA.y, anchorA.z };
	j["anchorB"] = { anchorB.x, anchorB.y, anchorB.z };

	j["axisA"] = { axisA.x, axisA.y, axisA.z };
	j["axisB"] = { axisB.x, axisB.y, axisB.z };

	j["normalA"] = { normalA.x, normalA.y, normalA.z };
	j["normalB"] = { normalB.x, normalB.y, normalB.z };

	j["minLimit"] = minLimit;
	j["maxLimit"] = maxLimit;

	j["springFrequency"] = springFrequency;
	j["springDamping"] = springDamping;

	j["maxFriction"] = maxFriction;

	j["motorEnabled"] = motorEnabled;
	j["motorTargetVelocity"] = motorTargetVelocity;
	j["motorMaxForce"] = motorMaxForce;

	return j;
}

void PhysicsConstraintComponent::Deserialize(const json& j)
{
	if (j.contains("connectedEntity"))
	{
		if (j["connectedEntity"].is_number())
		{
			connectedEntityUUID = j["connectedEntity"].get<EntityUUID>();
		}
		else if (j["connectedEntity"].is_string())
		{
			std::string uuidStr = j["connectedEntity"].get<std::string>();
			if (!uuidStr.empty())
				connectedEntityUUID = std::stoull(uuidStr);
		}
	}

	type = static_cast<ConstraintType>(j.value("type", static_cast<int>(ConstraintType::Fixed)));
	space = static_cast<ConstraintSpace>(j.value("space", static_cast<int>(ConstraintSpace::WorldSpace)));
	enabled = j.value("enabled", true);
	autoDetectPoints = j.value("autoDetectPoints", true);

	if (j.contains("anchorA") && j["anchorA"].is_array() && j["anchorA"].size() == 3)
		anchorA = glm::vec3(j["anchorA"][0], j["anchorA"][1], j["anchorA"][2]);

	if (j.contains("anchorB") && j["anchorB"].is_array() && j["anchorB"].size() == 3)
		anchorB = glm::vec3(j["anchorB"][0], j["anchorB"][1], j["anchorB"][2]);

	if (j.contains("axisA") && j["axisA"].is_array() && j["axisA"].size() == 3)
		axisA = glm::vec3(j["axisA"][0], j["axisA"][1], j["axisA"][2]);

	if (j.contains("axisB") && j["axisB"].is_array() && j["axisB"].size() == 3)
		axisB = glm::vec3(j["axisB"][0], j["axisB"][1], j["axisB"][2]);

	if (j.contains("normalA") && j["normalA"].is_array() && j["normalA"].size() == 3)
		normalA = glm::vec3(j["normalA"][0], j["normalA"][1], normalA[2]);

	if (j.contains("normalB") && j["normalB"].is_array() && j["normalB"].size() == 3)
		normalB = glm::vec3(j["normalB"][0], j["normalB"][1], j["normalB"][2]);

	minLimit = j.value("minLimit", 0.0f);
	maxLimit = j.value("maxLimit", 0.0f);

	springFrequency = j.value("springFrequency", 0.0f);
	springDamping = j.value("springDamping", 0.0f);

	maxFriction = j.value("maxFriction", 0.0f);

	motorEnabled = j.value("motorEnabled", false);
	motorTargetVelocity = j.value("motorTargetVelocity", 0.0f);
	motorMaxForce = j.value("motorMaxForce", 0.0f);

	shapeDirty = true;
}

}
