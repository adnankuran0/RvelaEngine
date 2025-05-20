#include "Renderer/Texture.h"
#include <string>
#include <unordered_map>
#include "FileUtils.h"

class TextureManager
{
public:
	static std::shared_ptr<Texture> LoadOrGetTexture(const Path path);
	static void UnloadTexture(const Path path);
	static void ClearTextures();
	static size_t GetTextureCount();
private:
	static std::unordered_map<std::string, std::shared_ptr<Texture>> textureMap;
};