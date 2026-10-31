#include "rvelapch.h"
#include "AudioClipLoader.h"
#include "Asset/Types/AudioClipAsset.h"
#include "Asset/AssetMeta.h"
#include "Core/Log.h"
#include "miniaudio/miniaudio.h"

using namespace rv;

Ref<Asset> AudioClipLoader::DecodeAudio(ma_decoder& decoder, const AssetUUID& uuid)
{
    ma_uint64 frameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS || frameCount == 0)
    {
        LOG_ERROR("Cant read audio clip length for UUID: {}", uuid.ToString());
        ma_decoder_uninit(&decoder);
        return nullptr;
    }

    ma_uint32 bpf = ma_get_bytes_per_frame(decoder.outputFormat, decoder.outputChannels);
    std::vector<uint8_t> pcm(frameCount * bpf);

    ma_uint64 framesRead = 0;
    ma_result result = ma_decoder_read_pcm_frames(&decoder, pcm.data(), frameCount, &framesRead);
    ma_decoder_uninit(&decoder);

    if (result != MA_SUCCESS || framesRead == 0)
    {
        LOG_ERROR("Cant decode audio clip for UUID: {}", uuid.ToString());
        return nullptr;
    }

    auto asset = CreateRef<AudioClipAsset>(uuid);
    asset->m_PCMData = std::move(pcm);
    asset->m_Format = decoder.outputFormat;
    asset->m_Channels = decoder.outputChannels;
    asset->m_SampleRate = decoder.outputSampleRate;
    asset->m_FrameCount = framesRead;

    return asset;
}

Ref<Asset> AudioClipLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;
    if (ma_decoder_init_file(assetPath.string().c_str(), &config, &decoder) != MA_SUCCESS)
    {
        LOG_ERROR("Audio clip cant opened: {}", assetPath.string());
        return nullptr;
    }

    return DecodeAudio(decoder, meta.uuid);
}

Ref<Asset> AudioClipLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size == 0)
    {
        LOG_ERROR("AudioClipLoader::LoadFromMemory: empty data for UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;
    if (ma_decoder_init_memory(data, size, &config, &decoder) != MA_SUCCESS)
    {
        LOG_ERROR("Audio clip memory decoding failed for UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    return DecodeAudio(decoder, meta.uuid);
}
