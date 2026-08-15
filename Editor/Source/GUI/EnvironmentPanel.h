#pragma once
#include <unordered_map>
#include <string>

namespace rv {

class Engine;

class EnvironmentPanel
{
public:
    EnvironmentPanel() = default;
    ~EnvironmentPanel() = default;

    void Draw(Engine* engine);

};

} // namespace rv