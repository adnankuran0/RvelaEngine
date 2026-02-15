#pragma once
#include <string>

namespace rv { 

class Layer
{
public:
	Layer(const std::string& name = "Layer") : m_DebugName(name) {}

	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate() {}
	virtual void OnFixedUpdate() {}
	virtual void OnLateUpdate() {}
	virtual void OnRender() {}

	inline const std::string& GetName() const noexcept { return m_DebugName; }

protected:
	std::string m_DebugName;
};

}