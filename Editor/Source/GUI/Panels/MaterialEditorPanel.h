#pragma once
#include "EditorPanel.h"
#include "Asset/AssetUUID.h"
#include "Asset/Types/MaterialAsset.h"
#include "Core/Ref.h"
#include <filesystem>
#include <string>

namespace rv {

class Engine;

class MaterialEditorPanel : public EditorPanel
{
public:
    MaterialEditorPanel();
    ~MaterialEditorPanel() override = default;

    void Draw(Engine* engine);

    static void OpenMaterial(AssetUUID uuid);
    static void OpenMaterial(const std::filesystem::path& path);
    static void SetMaterial(AssetUUID uuid);
    static AssetUUID GetCurrentMaterialUUID();
    static void Reset();

private:
    void LoadMaterial(AssetUUID uuid);
    bool SaveCurrentMaterial();
    void RevertCurrentMaterial();
    void MarkDirty(Engine* engine);

    static MaterialEditorPanel* s_Instance;

    bool m_IsDirty = false;
    bool m_AutoSave = false;

    AssetUUID m_CurrentMaterialUUID;
    Ref<MaterialAsset> m_CurrentMaterial;
    std::filesystem::path m_CurrentMaterialPath;
};

} // namespace rv