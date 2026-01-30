#include "rvelapch.h"
#include "UUIDComponent.h"
#include <iostream>

json UUIDComponent::Serialize() const
{
    return uuid;
}
void UUIDComponent::Deserialize(const json& j)
{
    uuid = j.get<EntityUUID>();
}