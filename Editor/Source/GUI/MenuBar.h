#pragma once

namespace rv {

class Engine;
class AssetImportPipeline;

class MenuBar
{
public:
	void Draw(Engine* engine, AssetImportPipeline& assetImporter);
};

}