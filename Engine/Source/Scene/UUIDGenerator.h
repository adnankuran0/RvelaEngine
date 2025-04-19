#pragma once
#include <cstdint>

using UUID = uint64_t;

class UUIDGenerator
{
public:
	static UUID Generate()
	{
		static UUID s_Current = 1;
		return s_Current++;
	}
};