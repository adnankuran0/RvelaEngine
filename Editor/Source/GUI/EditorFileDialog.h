#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace rv {

enum class FileDialogMode
{
    OpenFile,
    SaveFile,
    SelectFolder
};

class EditorFileDialog
{
public:
    using Callback = std::function<void(const std::filesystem::path&)>;

    static void Open(
        const std::string& title,
        FileDialogMode mode,
        const std::vector<std::string>& filters = {},
        const std::filesystem::path& initialDir = {},
        Callback onConfirm = nullptr,
        const std::string& defaultFileName = ""
    );

    static void Close();
    static bool IsOpen();
    static void Draw();

private:
    static EditorFileDialog& Get()
    {
        static EditorFileDialog instance;
        return instance;
    }

    EditorFileDialog() = default;
    ~EditorFileDialog() = default;

    void InternalOpen(
        const std::string& title,
        FileDialogMode mode,
        const std::vector<std::string>& filters,
        const std::filesystem::path& initialDir,
        Callback onConfirm,
        const std::string& defaultFileName
    );

    void InternalDraw();
    void RefreshFileList();
    void NavigateTo(const std::filesystem::path& newPath);
    void GoBack();
    void GoForward();
    void GoUp();
    void ConfirmSelection(const std::filesystem::path& targetPath);

    struct FileItem
    {
        std::string filename;
        std::filesystem::path path;
        bool isDirectory = false;
        std::string extension;
        uintmax_t fileSize = 0;
    };

    bool m_IsOpen = false;
    bool m_ShouldOpenPopup = false;
    std::string m_Title = "Select File";
    FileDialogMode m_Mode = FileDialogMode::OpenFile;
    std::vector<std::string> m_Filters;
    int m_SelectedFilterIndex = 0;
    std::filesystem::path m_CurrentDirectory;
    Callback m_OnConfirm = nullptr;

    std::vector<FileItem> m_CachedItems;
    int m_SelectedItemIndex = -1;
    char m_FileNameBuffer[256] = "";
    char m_SearchBuffer[128] = "";

    std::vector<std::filesystem::path> m_History;
    int m_HistoryIndex = -1;
};

}
