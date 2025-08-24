#include "rvelapch.h"
#include "Serializer.h"


void Serializer::SaveToFile(const ISerializable& obj, const std::string& path)
{
	try
	{
		std::filesystem::create_directories(std::filesystem::path(path).parent_path());

		std::ofstream file(path);
		if (!file.is_open())
		{
			LOG_ERROR("File can not be opened: {}", path);
			return;
		}

		std::string data = obj.Serialize();
		file << data;
		file.close();
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Serializer::SaveToFile error: {}", e.what());
	}
}
void Serializer::LoadFromFile(ISerializable& obj, const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOG_ERROR("File can not be opened: {}", path);
	}
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	obj.Deserialize(str.c_str());
}

