#include "AssetBrowserPanel.h"
#include "ImGui/imgui.h"

void AssetBrowserPanel::Draw()
{
    ImGui::Begin("Asset Browser" , nullptr , ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);
    {
        ImGui::Text("Assets/Models");
        ImGui::Separator();
        ImGui::Button("character.fbx", ImVec2(100, 100));
        ImGui::SameLine();
        ImGui::Button("environment.fbx", ImVec2(100, 100));
        ImGui::SameLine();
        ImGui::Button("weapon.fbx", ImVec2(100, 100));
    }
    ImGui::End();
}