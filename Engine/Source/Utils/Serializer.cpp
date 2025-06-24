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
			std::cerr << "File can not be opened: " << path << std::endl;
			return;
		}

		std::string data = obj.Serialize();
		file << data;
		file.close();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Serializer::SaveToFile error: " << e.what() << std::endl;
	}
}
void Serializer::LoadFromFile(ISerializable& obj, const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "File can not be opened: " << path << std::endl;
	}
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	obj.Deserialize(str.c_str());
}