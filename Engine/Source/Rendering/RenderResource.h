#pragma once
#include "GLAD/gl.h"
#include <string>
#include <unordered_map>

namespace rv {

enum class RenderResourceType
{
    Texture,
    Framebuffer
};

struct RenderResource
{
    RenderResourceType type;
    GLuint id;
};

class RenderResourceRegistry
{
public:
    void Register(const std::string& name, RenderResource resource);
    RenderResource* Get(const std::string& name);
    void Clear() { m_Resources.clear(); }

private:
    std::unordered_map<std::string, RenderResource> m_Resources;
};


}