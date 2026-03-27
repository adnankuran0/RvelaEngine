#pragma once

namespace rv { 

class ScreenQuad
{
public:
    void Draw();
    void Init();
    
private:
	unsigned int quadVAO;
	unsigned int  quadVBO;
};

}