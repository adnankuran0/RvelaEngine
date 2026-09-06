#pragma once
#include <unordered_map>
#include <string>

namespace rv {

class Texture;

enum class EditorIcon
{
	AnimLib,
	AudioEmitter,
	Camera,
	DirectionalLight,
	Folder,
	Material,
	Mesh,
	ParticleEmitter,
	PointLight,
	Prefab,
	Probe,
	Scene,
	Script,
	SpotLight,
	Texture,
	Audio,
	Play,
	Pause,
	Stop
};

class IconLibrary
{
public:
	static void Init();
	static void Shutdown();

	const Texture& GetIcon(EditorIcon icon) const;
	static IconLibrary& Get()
	{
		static IconLibrary instance;
		return instance;
	}
private:
	IconLibrary() = default;
	~IconLibrary() = default;

	void LoadIcon(EditorIcon icon, const std::string& relativePath);
	std::unordered_map<EditorIcon, Texture> m_Icons;
};

}