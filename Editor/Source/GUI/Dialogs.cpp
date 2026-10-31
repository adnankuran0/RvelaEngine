#include "Dialogs.h"
#include <ImGui/tinyfiledialogs.h>
#include "Utils/ProjectManager.h"

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <filesystem>
#include <thread>
#include <atomic>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#endif

using namespace rv;

std::string rv::Dialogs::OpenSceneDialog()
{
    constexpr const char* filterPatterns[] = { "*.rscene" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetAssetDirectory().string()
        : "";
    const char* filePath = tinyfd_openFileDialog("Select a scene", defaultPath.c_str(), 1, filterPatterns, nullptr, 0);
    return filePath ? filePath : "";
}

std::string rv::Dialogs::SaveSceneDialog()
{
    const char* filterPatterns[] = { "*.rscene" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? (ProjectManager::GetAssetDirectory() / "Scenes" / "scene.rscene").string()
        : "scene.rscene";
    const char* filePath = tinyfd_saveFileDialog("Save Scene As", defaultPath.c_str(), 1, filterPatterns, NULL);

    return filePath ? filePath : "";
}

std::string rv::Dialogs::OpenProjectDialog()
{
    constexpr const char* filterPatterns[] = { "*.rproj" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetProjectPath().string()
        : "";
    const char* filePath = tinyfd_openFileDialog("Select a Project", defaultPath.c_str(), 1, filterPatterns, nullptr, 0);
    return filePath ? filePath : "";
}

std::string rv::Dialogs::SelectFolderDialog(const std::string& title)
{
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetProjectPath().parent_path().string()
        : "";

#ifdef _WIN32
    std::string result;
    HWND hwndOwner = GetActiveWindow();
    if (!hwndOwner) hwndOwner = GetForegroundWindow();

    std::thread dlgThread([&result, title, defaultPath, hwndOwner]() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        IFileOpenDialog* pFileOpen = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen))))
        {
            DWORD dwOptions = 0;
            if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
            {
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            }

            std::wstring wTitle(title.begin(), title.end());
            pFileOpen->SetTitle(wTitle.c_str());

            if (!defaultPath.empty())
            {
                std::filesystem::path defP = defaultPath;
                std::wstring wDef = defP.wstring();
                IShellItem* pDefaultItem = nullptr;
                if (SUCCEEDED(SHCreateItemFromParsingName(wDef.c_str(), NULL, IID_PPV_ARGS(&pDefaultItem))))
                {
                    pFileOpen->SetFolder(pDefaultItem);
                    pDefaultItem->Release();
                }
            }

            if (SUCCEEDED(pFileOpen->Show(hwndOwner)))
            {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
                {
                    PWSTR pszFilePath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                    {
                        std::filesystem::path p(pszFilePath);
                        auto u8 = p.u8string();
                        result = std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }

        if (SUCCEEDED(hr))
        {
            CoUninitialize();
        }
    });

    HANDLE hThread = (HANDLE)dlgThread.native_handle();
    while (WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        MsgWaitForMultipleObjects(1, &hThread, FALSE, 20, QS_ALLINPUT);
    }

    if (dlgThread.joinable())
        dlgThread.join();

    return result;
#else
    const char* folderPath = tinyfd_selectFolderDialog(title.c_str(), defaultPath.c_str());
    return folderPath ? folderPath : "";
#endif
}
