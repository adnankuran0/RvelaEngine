#pragma once
#include "Scene/UUIDGenerator.h"
#include "Core/Ref.h"


class Asset : public RefCounted {
public:
    Asset() : uuid(UUIDGenerator::GeneratePersistent()) {}
    virtual ~Asset() = default;

    UUID GetUUID() const { return uuid; }
    void SetUUID(UUID id) { uuid = id; }

protected:
    UUID uuid;
};
