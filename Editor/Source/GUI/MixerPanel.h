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
        std::unordered_map<uint32_t, float> m_BusVolumes;

        uint32_t m_RenamingBusID = 0;
        char m_RenameBuffer[256] = "";
    };

} // namespace rv