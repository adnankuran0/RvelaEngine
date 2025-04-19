#pragma once
#include "Core/Utils/ISerializable.h"

class Component : public ISerializable
{
public:
	int ID = 0;
};