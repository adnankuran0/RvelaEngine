#pragma once
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "AssetImporter/AssetImporterRegistry.h"

class MenuBar
{
public:
	void Draw(Engine* engine, AssetImporterRegistry& assetImporter);
};

