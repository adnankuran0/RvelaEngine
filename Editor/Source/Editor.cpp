#include "Editor.h"


Editor::Editor()
{
    if (s_Instance == nullptr)
    {
        s_Instance = this;
        s_Engine = new Engine();
    }
    else
    {
        std::cout << "Another Editor instance is already created!";
    }
    

    Run();
}

void Editor::Run()
{
    s_Engine->Run();
}

Editor* Editor::GetInstance()
{
    return s_Instance;
}

Engine* Editor::GetEngine()
{
    return s_Engine;
}


