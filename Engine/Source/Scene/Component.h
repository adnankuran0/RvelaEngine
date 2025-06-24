#pragma once
#include "Utils/ISerializable.h"

class Component : public ISerializable
{
public:
	int ID = 0;
};