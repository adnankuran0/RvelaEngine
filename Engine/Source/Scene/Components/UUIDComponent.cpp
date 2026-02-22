#include "rvelapch.h"
#include "UUIDComponent.h"
#include <iostream>

using namespace rv;

json UUIDComponent::Serialize() const
{
    return uuid;
}
void UUIDComponent::Deserialize(const json& j)
{
    uuid = j.get<EntityUUID>();
}
