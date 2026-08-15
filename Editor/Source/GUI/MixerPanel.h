#pragma once
#include <unordered_map>
#include <string>

namespace rv {

    class MixerPanel
    {
    public:
        MixerPanel() = default;
        ~MixerPanel() = default;

        void Draw();

    private:
        uint32_t m_RenamingBusID = UINT32_MAX;
        char m_RenameBuffer[256] = "";
    };

} // namespace rv