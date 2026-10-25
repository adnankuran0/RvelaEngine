#include "Dockspace.h"
#include "ImGui/imgui.h"

using namespace rv;

const char* Dockspace::GetDefaultLayoutIni()
{
    static const char* s_DefaultLayoutIni =
        "[Window][Scene Hierarchy]\n"
        "Pos=0,64\n"
        "Size=320,603\n"
        "Collapsed=0\n"
        "DockId=0x0000001F,0\n"
        "\n"
        "[Window][Inspector]\n"
        "Pos=1593,64\n"
        "Size=327,603\n"
        "Collapsed=0\n"
        "DockId=0x0000001C,0\n"
        "\n"
        "[Window][Viewport]\n"
        "Pos=322,64\n"
        "Size=1269,603\n"
        "Collapsed=0\n"
        "DockId=0x0000000F,0\n"
        "\n"
        "[Window][DockSpace]\n"
        "Pos=0,64\n"
        "Size=1920,945\n"
        "Collapsed=0\n"
        "\n"
        "[Window][Asset Browser]\n"
        "Pos=0,669\n"
        "Size=1920,340\n"
        "Collapsed=0\n"
        "DockId=0x00000022,0\n"
        "\n"
        "[Window][Audio Mixer]\n"
        "Pos=0,669\n"
        "Size=1920,340\n"
        "Collapsed=0\n"
        "DockId=0x00000022,1\n"
        "\n"
        "[Window][Environment]\n"
        "Pos=1593,64\n"
        "Size=327,603\n"
        "Collapsed=0\n"
        "DockId=0x0000001C,1\n"
        "\n"
        "[Window][Animator]\n"
        "Pos=0,669\n"
        "Size=1920,340\n"
        "Collapsed=0\n"
        "DockId=0x00000022,2\n"
        "\n"
        "[Window][Console]\n"
        "Pos=0,669\n"
        "Size=1920,340\n"
        "Collapsed=0\n"
        "DockId=0x00000022,3\n"
        "\n"
        "[Window][Import]\n"
        "Pos=0,64\n"
        "Size=320,603\n"
        "Collapsed=0\n"
        "DockId=0x0000001F,1\n"
        "\n"
        "[Window][###MaterialEditor]\n"
        "Pos=1593,64\n"
        "Size=327,603\n"
        "Collapsed=0\n"
        "DockId=0x0000001C,2\n"
        "\n"
        "[Docking][Data]\n"
        "DockSpace                           ID=0x14621557 Window=0x3DA2F1DE Pos=0,87 Size=1920,945 Split=Y\n"
        "  DockNode                          ID=0x00000021 Parent=0x14621557 SizeRef=1920,603 Split=X\n"
        "    DockNode                        ID=0x0000001F Parent=0x00000021 SizeRef=320,945 Selected=0xB8729153\n"
        "    DockNode                        ID=0x00000020 Parent=0x00000021 SizeRef=1598,945 Split=X\n"
        "      DockNode                      ID=0x00000003 Parent=0x00000020 SizeRef=265,945 Selected=0xB8729153\n"
        "      DockNode                      ID=0x00000004 Parent=0x00000020 SizeRef=1653,945 Split=X\n"
        "        DockNode                    ID=0x0000001D Parent=0x00000004 SizeRef=1423,945 Split=X\n"
        "          DockNode                  ID=0x0000001B Parent=0x0000001D SizeRef=1269,950 Split=X\n"
        "            DockNode                ID=0x00000014 Parent=0x0000001B SizeRef=253,950 Selected=0xBABDAE5E\n"
        "            DockNode                ID=0x0000001A Parent=0x0000001B SizeRef=1665,950 Split=X\n"
        "              DockNode              ID=0x00000013 Parent=0x0000001A SizeRef=190,950 Selected=0xEB6F2C76\n"
        "              DockNode              ID=0x00000019 Parent=0x0000001A SizeRef=1728,950 Split=X\n"
        "                DockNode            ID=0x00000015 Parent=0x00000019 SizeRef=1484,1002 Split=Y\n"
        "                  DockNode          ID=0x00000011 Parent=0x00000015 SizeRef=1920,46 Selected=0x315A5568\n"
        "                  DockNode          ID=0x00000012 Parent=0x00000015 SizeRef=1920,773 Split=Y\n"
        "                    DockNode        ID=0x00000007 Parent=0x00000012 SizeRef=1920,86\n"
        "                    DockNode        ID=0x00000009 Parent=0x00000012 SizeRef=1920,992 Split=Y\n"
        "                      DockNode      ID=0x0000000B Parent=0x00000009 SizeRef=1920,792 Split=X\n"
        "                        DockNode    ID=0x0000000D Parent=0x0000000B SizeRef=383,792\n"
        "                        DockNode    ID=0x0000000E Parent=0x0000000B SizeRef=1535,792 Split=X\n"
        "                          DockNode  ID=0x0000000F Parent=0x0000000E SizeRef=1226,792 CentralNode=1 Selected=0xC450F867\n"
        "                          DockNode  ID=0x00000010 Parent=0x0000000E SizeRef=307,792\n"
        "                      DockNode      ID=0x0000000C Parent=0x00000009 SizeRef=1920,198\n"
        "                DockNode            ID=0x00000016 Parent=0x00000019 SizeRef=242,1002 Selected=0x36DC96AB\n"
        "          DockNode                  ID=0x0000001C Parent=0x0000001D SizeRef=327,950 Selected=0x36DC96AB\n"
        "        DockNode                    ID=0x0000001E Parent=0x00000004 SizeRef=495,945 Selected=0x8C72BEA8\n"
        "  DockNode                          ID=0x00000022 Parent=0x14621557 SizeRef=1920,340 Selected=0xB6C74292\n";

    return s_DefaultLayoutIni;
}

void Dockspace::ResetLayout()
{
    ImGui::LoadIniSettingsFromMemory(GetDefaultLayoutIni());
    ImGuiIO& io = ImGui::GetIO();
    if (io.IniFilename)
    {
        ImGui::SaveIniSettingsToDisk(io.IniFilename);
    }
}

void Dockspace::Draw()
{
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    // Docking alanı oluşturma
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();
}