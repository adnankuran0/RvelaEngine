#include "rvelapch.h"
#include "AudioBus.h"

using namespace rv;

AudioBus::AudioBus(ma_engine* engine, ma_sound_group* parent)
{
    ma_result result = ma_sound_group_init(
        engine,
        0,
        parent,
        &m_Group
    );

    assert(result == MA_SUCCESS);
}

AudioBus::~AudioBus()
{
    ma_sound_group_uninit(&m_Group);
}

ma_sound_group* AudioBus::GetGroup()
{
    return &m_Group;
}

