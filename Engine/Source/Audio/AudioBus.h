#pragma once
#include "miniaudio/miniaudio.h"
#include <string>

namespace rv {

class AudioBus
{
public:
	AudioBus(ma_engine* engine, ma_sound_group* parent);
	~AudioBus();

	const std::string& GetName() { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	unsigned int GetID() { return m_ID; }
	void SetID(unsigned int id) { m_ID = id; }

	unsigned int GetParentBusID() { return m_ParentBus; }
	void SetParentBusID(unsigned int parentBusID ) { m_ParentBus = parentBusID; }

	ma_sound_group* GetGroup();

	void SetVolume(float volume);
private:
	unsigned int m_ID;
	unsigned int m_ParentBus = 0; // master bus by default
	std::string m_Name;
	ma_sound_group m_Group{};
};

}