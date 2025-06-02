#pragma once

class EditorContext
{
public:
	static void SetFinalTexture(unsigned int finalTexture) { finalTexture = FinalTexture; }
	static unsigned int GetFinalTexture() { return FinalTexture; }
private:
	static unsigned int FinalTexture;
};