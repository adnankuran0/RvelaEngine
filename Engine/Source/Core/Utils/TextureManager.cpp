#include "rvelapch.h"
#include "TextureManager.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureManager::textureMap;

std::shared_ptr<Texture> TextureManager::LoadOrGetTexture(const std::string& path)
{
	if (path == "") return nullptr;

	auto it = textureMap.find(path);
	if (it != textureMap.end())
		return it->second;

	auto texture = std::make_shared<Texture>();
	texture->GenerateFromImage(path);
	textureMap[path] = texture;
	return texture;
}

void TextureManager::UnloadTexture(const std::string& path) {
	if (path == "") return;

	auto it = textureMap.find(path);
	if (it != textureMap.end()) {
		textureMap.erase(it); 
	}
	else {
		std::cout << "Texture not found: " << path << std::endl;
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
