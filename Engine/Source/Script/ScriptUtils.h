#pragma once

#include "sol/sol.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace rv {

using json = nlohmann::json;

namespace ScriptUtils {

    json SolObjectToJson(const sol::object& obj);
    void JsonToSolObject(const json& j, sol::table& target, const std::string& key);

    json TableToJson(const sol::table& table);
    void JsonToTable(const json& j, sol::table& target);

    json SerializeLuaInstanceData(const sol::table& instance);
    void DeserializeLuaInstanceData(const json& j, sol::table& instance);

} // namespace ScriptUtils

} // namespace rv
