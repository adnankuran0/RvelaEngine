#pragma once
#include "Utils/ISerializable.h"

class Serializer
{
public:
	static void SaveToFile(const ISerializable& obj, const std::string& path);
	static void LoadFromFile(ISerializable& obj, const std::string& path);
};
