#pragma once

namespace rv {

class Engine;
class AssetImporterRegistry;

class MenuBar
{
public:
	void Draw(Engine* engine, AssetImporterRegistry& assetImporter);
};

}