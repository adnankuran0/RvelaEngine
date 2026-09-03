#pragma once
#include "entt/entt.h"
#include "Asset/AssetUUID.h"

namespace rv::Audio {

enum class EventType
{
    FINISHED
};

struct AudioDispatchEvent
{
    entt::entity entity;
    EventType type;
    AssetUUID clipUUID;
};

}