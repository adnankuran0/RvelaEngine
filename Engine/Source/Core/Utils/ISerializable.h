#pragma once
#include "rvelapch.h"
class ISerializable
{
public:
	virtual ~ISerializable() = default;
	virtual std::string Serialize() const = 0;
	virtual void Deserialize(const std::string& jsonStr) = 0;
};
