#include "rvelapch.h"
#include "TextureManager.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureManager::textureMap;

std::shared_ptr<Texture> TextureManager::LoadOrGetTexture(const Path path)
{
	if (!path.IsValid()) return nullptr;

	auto it = textureMap.find(path.GetAbsoluteStr());
	if (it != textureMap.end())
		return it->second;


	auto texture = std::make_shared<Texture>();
	texture->GenerateFromImage(path.GetAbsoluteStr());
	textureMap[path.GetAbsoluteStr()] = texture;
	return texture;
}

void TextureManager::UnloadTexture(const Path path) {
	if (!path.IsValid()) return;

	auto it = textureMap.find(path.GetAbsoluteStr());
	if (it != textureMap.end()) {
		textureMap.erase(it); 
	}
	else {
		std::cout << "Unload failed: Texture not found: " << path.GetAbsoluteStr() << std::endl;
	}
}

void TextureManager::ClearTextures()
{
	textureMap.clear();
	std::cout << "All textures cleared." << std::endl;
}

size_t TextureManager::GetTextureCount() 
{
	return textureMap.size();
}
