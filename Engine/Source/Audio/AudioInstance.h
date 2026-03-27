#pragma once
#include "miniaudio/miniaudio.h"
#include "entt/entt.h"

namespace rv {


struct AudioInstance
{
	ma_sound sound{};
	entt::entity entity{};
};

}