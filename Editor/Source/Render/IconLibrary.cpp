#include "IconLibrary.h"
#include "Renderer/Texture.h"
#include "Utils/FileUtils.h"
#include <cassert>

using namespace rv;

void IconLibrary::LoadIcon(EditorIcon icon, const std::string& relativePath)
{
    auto [it, inserted] = m_Icons.try_emplace(icon);
    if (inserted)
    {
        Texture& tex = it->second;
        tex.Init();
        tex.GenerateFromImage(EDITOR_PATH(relativePath).GetAbsoluteStr());
    }
}

void IconLibrary::Init()
{
    auto& lib = Get();
    lib.m_Icons.clear();

    lib.LoadIcon(EditorIcon::AnimLib, "Icons\\animlib.png");
    lib.LoadIcon(EditorIcon::AudioEmitter, "Icons\\audioemitter.png");
    lib.LoadIcon(EditorIcon::Camera, "Icons\\camera.png");
    lib.LoadIcon(EditorIcon::DirectionalLight, "Icons\\directionallight.png");
    lib.LoadIcon(EditorIcon::Folder, "Icons\\folder.png");
    lib.LoadIcon(EditorIcon::Material, "Icons\\material.png");
    lib.LoadIcon(EditorIcon::Mesh, "Icons\\mesh.png");
    lib.LoadIcon(EditorIcon::ParticleEmitter, "Icons\\particleemitter.png");
    lib.LoadIcon(EditorIcon::PointLight, "Icons\\pointlight.png");
    lib.LoadIcon(EditorIcon::Prefab, "Icons\\prefab.png");
    lib.LoadIcon(EditorIcon::Probe, "Icons\\probe.png");
    lib.LoadIcon(EditorIcon::Scene, "Icons\\scene.png");
    lib.LoadIcon(EditorIcon::Script, "Icons\\script.png");
    lib.LoadIcon(EditorIcon::SpotLight, "Icons\\spotlight.png");
    lib.LoadIcon(EditorIcon::Texture, "Icons\\texture.png");
    lib.LoadIcon(EditorIcon::Audio, "Icons\\audio.png");
    lib.LoadIcon(EditorIcon::Play, "Icons\\play.png");
    lib.LoadIcon(EditorIcon::Pause, "Icons\\pause.png");
    lib.LoadIcon(EditorIcon::Stop, "Icons\\stop.png");

}

void IconLibrary::Shutdown()
{
    for (auto& [type, tex] : Get().m_Icons)
    {
        tex.Destroy();
    }
    Get().m_Icons.clear();
}

const Texture& IconLibrary::GetIcon(EditorIcon icon) const
{
    auto it = m_Icons.find(icon);
    if (it != m_Icons.end())
    {
        return it->second;
    }

    assert(false && "Cant find editor icon!");
    static Texture emptyFallback;
    return emptyFallback;
}