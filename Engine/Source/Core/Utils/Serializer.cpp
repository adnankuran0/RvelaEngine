#include "rvelapch.h"
#include "Serializer.h"


void Serializer::SaveToFile(const ISerializable& obj, const std::string& path)
{
	std::ofstream file(path);
	if (!file.is_open())
	{
		std::cerr << "Dosya açılamadı: " << path << std::endl;
		return;
	}
	std::string data = obj.Serialize();
	file << data.c_str();
}

void Serializer::LoadFromFile(ISerializable& obj, const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "Dosya açılamadı: " << path << std::endl;
	}
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	obj.Deserialize(str.c_str());
}