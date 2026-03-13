#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

namespace rv::Physics
{

struct CharacterContactKey
{
    const JPH::CharacterVirtual* character;
    JPH::BodyID bodyID;

    bool operator==(const CharacterContactKey& o) const
    {
        return character == o.character && bodyID == o.bodyID;
    }
};

struct CharacterContactKeyHash
{
    size_t operator()(const CharacterContactKey& k) const
    {
        size_t h1 = std::hash<const void*>{}(k.character);
        size_t h2 = std::hash<uint32_t>{}(k.bodyID.GetIndex());
        return h1 ^ (h2 << 32);
    }
};

}