#include "rvelapch.h"
#if 0
#include "TextureManager.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureManager::textureMap;

std::shared_ptr<Texture> TextureManager::LoadOrGetTexture(const Path path)
{
	if (!path.IsValid()) return nullptr;

	auto absPath = path.GetAbsoluteStr();

	auto it = textureMap.find(absPath);
	if (it != textureMap.end())
		return it->second;


	auto texture = std::make_shared<Texture>();
	texture->GenerateFromImage(absPath);
	textureMap[absPath] = texture;
	return texture;
}

void TextureManager::UnloadTexture(const Path path) {
	/*if (!path.IsValid()) return;

	auto it = textureMap.find(path.GetAbsoluteStr());
	if (it != textureMap.end()) {
		textureMap.erase(it); 
	}
	else {
		LOG_WARN("Unload failed: Texture not found: {}", path.GetAbsoluteStr());
	}*/
}

void TextureManager::ClearTextures()
{
	textureMap.clear();
}

size_t TextureManager::GetTextureCount() 
{
	return textureMap.size();
}
#endif