#pragma once

namespace rv {

class Dockspace
{
public:
	void Draw();
	static const char* GetDefaultLayoutIni();
	static void ResetLayout();
};

}