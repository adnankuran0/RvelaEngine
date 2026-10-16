#include "rvelapch.h"
#include "ScriptUtils.h"
#include <glm/glm.hpp>
#include <unordered_set>

namespace rv::ScriptUtils {

static const std::unordered_set<std::string> s_IgnoredInstanceKeys = {
    "entity", "scene", "physics",
    "EmitSignal", "StartCoroutine", "StopCoroutine",
    "StartTimer", "StartRepeatingTimer", "CancelTimer",
    "TweenPosition", "TweenRotation", "TweenScale",
    "TweenValue", "Tween", "CancelTweens",
    "OnCreate", "OnReady", "OnUpdate",
    "OnFixedUpdate", "OnLateUpdate", "OnDestroy",
    "OnEnabled", "OnDisabled", "OnScenePaused",
    "OnSceneResumed", "OnCollisionEnter", "OnCollisionStay",
    "OnCollisionExit", "OnTriggerEnter", "OnTriggerStay",
    "OnTriggerExit", "OnAnimationEvent", "OnAnimationStarted",
    "OnAnimationFinished", "OnAnimationLooped", "OnAudioFinished"
};

json SolObjectToJson(const sol::object& obj)
{
    if (!obj.valid() || obj.is<sol::nil_t>()) return json();
    if (obj.is<bool>()) return obj.as<bool>();
    if (obj.is<int64_t>()) return obj.as<int64_t>();
    if (obj.is<int32_t>()) return obj.as<int32_t>();
    if (obj.is<double>()) return obj.as<double>();
    if (obj.is<float>()) return obj.as<float>();
    if (obj.is<std::string>()) return obj.as<std::string>();
    if (obj.is<glm::vec2>()) { auto v = obj.as<glm::vec2>(); return json::array({ v.x, v.y }); }
    if (obj.is<glm::vec3>()) { auto v = obj.as<glm::vec3>(); return json::array({ v.x, v.y, v.z }); }
    if (obj.is<glm::vec4>()) { auto v = obj.as<glm::vec4>(); return json::array({ v.x, v.y, v.z, v.w }); }
    if (obj.is<sol::table>())
    {
        return TableToJson(obj.as<sol::table>());
    }
    return json();
}

json TableToJson(const sol::table& table)
{
    bool isArray = true;
    size_t count = 0;
    for (const auto& [k, v] : table)
    {
        count++;
        if (!k.is<size_t>() && !k.is<int>()) { isArray = false; break; }
    }

    if (isArray && count > 0)
    {
        json arr = json::array();
        for (size_t i = 1; i <= count; ++i)
        {
            sol::object elem = table[i];
            if (elem.valid()) arr.push_back(SolObjectToJson(elem));
            else arr.push_back(nullptr);
        }
        return arr;
    }
    else
    {
        json tbl = json::object();
        for (const auto& [k, v] : table)
        {
            if (k.is<std::string>())
            {
                std::string key = k.as<std::string>();
                if (v.get_type() != sol::type::function)
                {
                    json sub = SolObjectToJson(v);
                    if (!sub.is_null()) tbl[key] = sub;
                }
            }
        }
        return tbl;
    }
}

void JsonToSolObject(const json& j, sol::table& target, const std::string& key)
{
    if (j.is_boolean()) target[key] = j.get<bool>();
    else if (j.is_number_integer()) target[key] = j.get<int64_t>();
    else if (j.is_number_float()) target[key] = j.get<double>();
    else if (j.is_string()) target[key] = j.get<std::string>();
    else if (j.is_array())
    {
        sol::state_view lua(target.lua_state());
        sol::table arr = lua.create_table();
        for (size_t i = 0; i < j.size(); ++i)
        {
            if (j[i].is_boolean()) arr[i + 1] = j[i].get<bool>();
            else if (j[i].is_number_integer()) arr[i + 1] = j[i].get<int64_t>();
            else if (j[i].is_number_float()) arr[i + 1] = j[i].get<double>();
            else if (j[i].is_string()) arr[i + 1] = j[i].get<std::string>();
            else if (j[i].is_object() || j[i].is_array())
            {
                JsonToSolObject(j[i], arr, std::to_string(i + 1));
            }
        }
        target[key] = arr;
    }
    else if (j.is_object())
    {
        sol::state_view lua(target.lua_state());
        sol::table subTable = lua.create_table();
        for (const auto& [subK, subV] : j.items())
        {
            JsonToSolObject(subV, subTable, subK);
        }
        target[key] = subTable;
    }
}

void JsonToTable(const json& j, sol::table& target)
{
    if (!j.is_object()) return;
    for (const auto& [k, v] : j.items())
    {
        JsonToSolObject(v, target, k);
    }
}

json SerializeLuaInstanceData(const sol::table& instance)
{
    json data = json::object();
    if (!instance.valid()) return data;

    for (const auto& [k, v] : instance)
    {
        if (!k.is<std::string>()) continue;
        std::string key = k.as<std::string>();

        if (s_IgnoredInstanceKeys.contains(key))
            continue;
        if (v.get_type() == sol::type::function)
            continue;

        json jVal = SolObjectToJson(v);
        if (!jVal.is_null())
            data[key] = jVal;
    }

    return data;
}

void DeserializeLuaInstanceData(const json& j, sol::table& instance)
{
    if (!j.is_object() || !instance.valid()) return;

    for (const auto& [k, v] : j.items())
    {
        JsonToSolObject(v, instance, k);
    }
}

} // namespace rv::ScriptUtils
