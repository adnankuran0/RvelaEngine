#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

namespace rv::Physics
{

struct ContactKey
{
    JPH::BodyID a;
    JPH::BodyID b;

    ContactKey(JPH::BodyID idA, JPH::BodyID idB)
    {
        if (idA < idB)
        {
            a = idA;
            b = idB;
        }
        else
        {
            a = idB;
            b = idA;
        }
    }

    bool operator==(const ContactKey& other) const
    {
        return a == other.a && b == other.b;
    }
};

struct ContactKeyHash
{
    size_t operator()(const ContactKey& k) const
    {
        return ((size_t)k.a.GetIndex() << 32) ^ (size_t)k.b.GetIndex(); 
    }
};

}
