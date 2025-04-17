#pragma once
#include "Core/ISerializable.h"
#include <fstream>

class Serializer
{
public:
	static void SaveToFile(const ISerializable& obj, const std::string& path);
	static void LoadFromFile(ISerializable& obj, const std::string& path);
};
