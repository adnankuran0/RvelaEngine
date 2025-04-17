#include "Texture.h"
#include <string>
#include <unordered_map>
class TextureManager
{
public:
	static std::shared_ptr<Texture> LoadOrGetTexture(const std::string& path);
	static void UnloadTexture(const std::string& path);
	static void ClearTextures();
	static size_t GetTextureCount();
private:
	static std::unordered_map<std::string, std::shared_ptr<Texture>> textureMap;
};