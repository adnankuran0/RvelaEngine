#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

namespace rv {

class AudioBus;

class MixerPanel
{
public:
    MixerPanel() = default;
    ~MixerPanel() = default;

    void Draw();

private:
    void DrawEffectsRack(AudioBus* bus);

    uint32_t m_RenamingBusID = UINT32_MAX;
    char m_RenameBuffer[256] = "";
    uint32_t m_SelectedBusID = 0;
    bool m_ShowEffectsRack = true;
};

}
