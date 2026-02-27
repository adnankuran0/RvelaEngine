#include "Dialogs.h"
#include <ImGui/tinyfiledialogs.h>

using namespace rv;

std::string rv::Dialogs::OpenSceneDialog()
{
    constexpr const char* filterPatterns[] = { "*.rscene" };
    const char* filePath = tinyfd_openFileDialog("Select a scene", "", 1, filterPatterns, nullptr, 0);
    return filePath ? filePath : "";
}

