#pragma once
#include "sol/state.hpp"
#include "ScriptProperty.h"
#include "Asset/AssetUUID.h"
#include <vector>
#include <unordered_map>

namespace rv {

class ScriptEngine
{
public:
	void Init();
	sol::state& GetState() { return m_State; }

	static std::vector<ScriptPropertyDef> ExtractProperties(const std::string& scriptSource);
	static const std::vector<ScriptPropertyDef>& GetScriptPropertyDefs(const AssetUUID& scriptUUID);
	static void InvalidateScriptPropertyDefs(const AssetUUID& scriptUUID);
	static void ClearScriptPropertyDefs();

private:
	void InitOverrides();
	sol::state m_State;
	static std::unordered_map<AssetUUID, std::vector<ScriptPropertyDef>> s_PropertyCache;
};

}