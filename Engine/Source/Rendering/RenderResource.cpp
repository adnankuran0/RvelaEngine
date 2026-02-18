#include "rvelapch.h"
#include "RenderResource.h"

namespace rv {

void RenderResourceRegistry::Register(const std::string& name, RenderResource resource)
{
    m_Resources[name] = resource;
}

RenderResource* RenderResourceRegistry::Get(const std::string& name)
{
    auto it = m_Resources.find(name);
    if (it == m_Resources.end())
        return nullptr;

    return &it->second;
}

}