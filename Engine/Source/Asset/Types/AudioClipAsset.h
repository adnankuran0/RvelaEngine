#pragma once
#include "Asset/Asset.h"
#include "miniaudio/miniaudio.h"
#include <vector>

namespace rv {

    class AudioClipAsset : public Asset
    {
    public:
        explicit AudioClipAsset(AssetUUID uuid) : Asset(uuid) {}

        bool IsValid() const { return !m_PCMData.empty(); }

        ma_format GetFormat()     const { return m_Format; }
        ma_uint32 GetChannels()   const { return m_Channels; }
        ma_uint32 GetSampleRate() const { return m_SampleRate; }
        ma_uint64 GetFrameCount() const { return m_FrameCount; }
        float     GetDuration()   const { return m_SampleRate ? (float)m_FrameCount / (float)m_SampleRate : 0.0f; }

        ma_result InitBufferRef(ma_audio_buffer_ref* pOut) const
        {
            return ma_audio_buffer_ref_init(m_Format, m_Channels, m_PCMData.data(), m_FrameCount, pOut);
        }

    private:
        friend class AudioClipLoader;

        std::vector<uint8_t> m_PCMData;
        ma_format m_Format = ma_format_unknown;
        ma_uint32 m_Channels = 0;
        ma_uint32 m_SampleRate = 0;
        ma_uint64 m_FrameCount = 0;
    };

}