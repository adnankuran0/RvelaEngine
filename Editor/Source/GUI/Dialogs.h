#pragma once
#include <string>


namespace rv
{

class Engine;
class AssetImporterRegistry;

class Dialogs {
public:
	static std::string OpenSceneDialog();
	static std::string SaveSceneDialog();
	static std::string OpenProjectDialog();
	static std::string SelectFolderDialog(const std::string& title = "Select Folder");
};


}