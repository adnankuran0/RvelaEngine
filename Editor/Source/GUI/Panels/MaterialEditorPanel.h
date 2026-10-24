#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/MaterialAsset.h"
#include "Core/Ref.h"
#include <filesystem>
#include <string>

namespace rv {

class Engine;

class MaterialEditorPanel
{
public:
    MaterialEditorPanel();
    ~MaterialEditorPanel() = default;

    void Draw(Engine* engine);

    void Open() { m_IsOpen = true; }
    void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }
    void Toggle() { m_IsOpen = !m_IsOpen; }

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

    bool m_IsOpen = true;
    bool m_IsDirty = false;
    bool m_AutoSave = false;

    AssetUUID m_CurrentMaterialUUID;
    Ref<MaterialAsset> m_CurrentMaterial;
    std::filesystem::path m_CurrentMaterialPath;
};

} // namespace rv
