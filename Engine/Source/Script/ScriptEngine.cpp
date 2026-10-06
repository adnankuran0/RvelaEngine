#include "rvelapch.h"
#include "ScriptEngine.h"
#include "ScriptBindings.h"
#include "Core/Engine.h"
#include "sol/variadic_args.hpp"
#include "Core/EditorConsoleSink.h"
#include "Asset/AssetManager.h"
#include "Asset/Types/ScriptAsset.h"
#include <algorithm>
#include <cctype>

using namespace rv;

std::unordered_map<AssetUUID, std::vector<ScriptPropertyDef>> ScriptEngine::s_PropertyCache;

static int SafeLuaPanic(lua_State* L)
{
    const char* message = lua_tostring(L, -1);
    std::cerr << "[LUA PANIC] " << (message ? message : "unknown") << std::endl;
    return 0;
}

void ScriptEngine::Init()
{
    m_State.set_panic(sol::c_call<decltype(&SafeLuaPanic), &SafeLuaPanic>);
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os, sol::lib::coroutine);

    m_State.script("function await(seconds) return coroutine.yield(math.max(0, seconds or 0)) end");

    InitOverrides();

    LuaBindings::RegisterMath(m_State);
    LuaBindings::RegisterCoreTypes(m_State);
    LuaBindings::RegisterDebugAPI(m_State);
    LuaBindings::RegisterComponents(m_State);
    LuaBindings::RegisterInputAPI(m_State);
    Engine* engine = Engine::Get();
    LuaBindings::RegisterSceneAPI(m_State, engine ? &engine->GetSceneManager() : nullptr);
    LuaBindings::RegisterPhysicsAPI(m_State);
    LuaBindings::RegisterAudioAPI(m_State);
    LuaBindings::RegisterAnimationAPI(m_State);
    LuaBindings::RegisterUIComponents(m_State);

}

void ScriptEngine::InitOverrides()
{
    m_State["print"] = [](sol::variadic_args va) {
        std::stringstream ss;
        bool first = true;

        for (auto v : va)
        {
            if (!first)
                ss << "\t";

            sol::state_view lua_state = va.lua_state();
            std::string str = lua_state["tostring"](v);
            ss << str;
            first = false;
        }

        std::string output = ss.str();

        std::cout << "[LUA] " << output << std::endl;

        EditorConsoleSink::Get().LogLua(output);
        };
}


static void InferPropertyFromValue(ScriptPropertyDef& def, const sol::object& val)
{
    if (!val.valid())
        return;

    sol::type t = val.get_type();
    if (t == sol::type::boolean)
    {
        def.type = ScriptPropertyType::Bool;
        def.boolVal = val.as<bool>();
    }
    else if (t == sol::type::number)
    {
        lua_State* L = val.lua_state();
        val.push();
        if (lua_isinteger(L, -1))
        {
            def.type = ScriptPropertyType::Int;
            def.intVal = val.as<int>();
            def.floatVal = static_cast<float>(def.intVal);
        }
        else
        {
            def.type = ScriptPropertyType::Float;
            def.floatVal = val.as<float>();
            def.intVal = static_cast<int>(def.floatVal);
        }
        lua_pop(L, 1);
    }
    else if (t == sol::type::string)
    {
        std::string s = val.as<std::string>();
        if (s.length() == 36 && s[8] == '-' && s[13] == '-' && s[18] == '-' && s[23] == '-')
        {
            AssetUUID parsed = AssetUUID::FromString(s);
            if (parsed.IsValid())
            {
                def.type = ScriptPropertyType::AssetHandle;
                def.assetVal = parsed;
                return;
            }
        }
        def.type = ScriptPropertyType::String;
        def.stringVal = s;
    }
    else if (t == sol::type::userdata)
    {
        if (val.is<AssetHandle>())
        {
            def.type = ScriptPropertyType::AssetHandle;
            def.assetVal = val.as<AssetHandle>();
        }
        else if (val.is<AssetHandle*>())
        {
            AssetHandle* ptr = val.as<AssetHandle*>();
            if (ptr)
            {
                def.type = ScriptPropertyType::AssetHandle;
                def.assetVal = *ptr;
            }
        }
        else if (val.is<glm::vec2>())
        {
            def.type = ScriptPropertyType::Vec2;
            def.vec2Val = val.as<glm::vec2>();
        }
        else if (val.is<glm::vec2*>())
        {
            glm::vec2* ptr = val.as<glm::vec2*>();
            if (ptr)
            {
                def.type = ScriptPropertyType::Vec2;
                def.vec2Val = *ptr;
            }
        }
        else if (val.is<glm::vec3>())
        {
            def.type = ScriptPropertyType::Vec3;
            def.vec3Val = val.as<glm::vec3>();
        }
        else if (val.is<glm::vec3*>())
        {
            glm::vec3* ptr = val.as<glm::vec3*>();
            if (ptr)
            {
                def.type = ScriptPropertyType::Vec3;
                def.vec3Val = *ptr;
            }
        }
        else if (val.is<glm::vec4>())
        {
            def.type = ScriptPropertyType::Vec4;
            def.vec4Val = val.as<glm::vec4>();
        }
        else if (val.is<glm::vec4*>())
        {
            glm::vec4* ptr = val.as<glm::vec4*>();
            if (ptr)
            {
                def.type = ScriptPropertyType::Vec4;
                def.vec4Val = *ptr;
            }
        }
        else
        {
            sol::state_view sv(val.lua_state());
            sol::function toStr = sv["tostring"];
            if (toStr.valid())
            {
                std::string s = toStr(val);
                if (s.rfind("Vec2(", 0) == 0)
                {
                    def.type = ScriptPropertyType::Vec2;
                    float x = 0.0f, y = 0.0f;
                    if (sscanf(s.c_str(), "Vec2(%f, %f)", &x, &y) >= 2)
                        def.vec2Val = glm::vec2(x, y);
                }
                else if (s.rfind("Vec3(", 0) == 0)
                {
                    def.type = ScriptPropertyType::Vec3;
                    float x = 0.0f, y = 0.0f, z = 0.0f;
                    if (sscanf(s.c_str(), "Vec3(%f, %f, %f)", &x, &y, &z) >= 3)
                        def.vec3Val = glm::vec3(x, y, z);
                }
                else if (s.rfind("Vec4(", 0) == 0)
                {
                    def.type = ScriptPropertyType::Vec4;
                    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
                    if (sscanf(s.c_str(), "Vec4(%f, %f, %f, %f)", &x, &y, &z, &w) >= 4)
                        def.vec4Val = glm::vec4(x, y, z, w);
                }
                else if (s.length() == 36 && s[8] == '-' && s[13] == '-' && s[18] == '-' && s[23] == '-')
                {
                    def.type = ScriptPropertyType::AssetHandle;
                    def.assetVal = AssetUUID::FromString(s);
                }
            }
        }
    }
}

static void ParsePropertyTableItem(ScriptPropertyDef& def, const sol::table& item)
{
    std::string typeStr;
    sol::object typeObj = item["type"];
    if (typeObj.valid() && typeObj.is<std::string>())
    {
        typeStr = typeObj.as<std::string>();
        std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    }

    sol::object minObj = item["min"];
    if (minObj.valid() && minObj.is<float>()) { def.minVal = minObj.as<float>(); def.hasRange = true; }

    sol::object maxObj = item["max"];
    if (maxObj.valid() && maxObj.is<float>()) { def.maxVal = maxObj.as<float>(); def.hasRange = true; }

    sol::object stepObj = item["step"];
    if (stepObj.valid() && stepObj.is<float>()) { def.step = stepObj.as<float>(); }

    sol::object defObj = item["default"];

    if (typeStr == "asset" || typeStr == "assethandle" || typeStr == "prefab" || typeStr == "material" || typeStr == "texture" || typeStr == "audio")
    {
        def.type = ScriptPropertyType::AssetHandle;
        if (defObj.valid())
        {
            if (defObj.is<std::string>())
            {
                std::string s = defObj.as<std::string>();
                if (s.length() == 36 && s[8] == '-' && s[13] == '-' && s[18] == '-' && s[23] == '-')
                    def.assetVal = AssetUUID::FromString(s);
                else
                    def.assetVal = AssetManager::Get().GetRegistry().GetUUID(s);
            }
            else if (defObj.is<AssetHandle>())
            {
                def.assetVal = defObj.as<AssetHandle>();
            }
            else if (defObj.is<AssetHandle*>())
            {
                AssetHandle* ptr = defObj.as<AssetHandle*>();
                if (ptr) def.assetVal = *ptr;
            }
        }
    }
    else if (typeStr == "float" || typeStr == "number")
    {
        def.type = ScriptPropertyType::Float;
        if (defObj.valid() && defObj.is<float>()) def.floatVal = defObj.as<float>();
    }
    else if (typeStr == "int" || typeStr == "integer")
    {
        def.type = ScriptPropertyType::Int;
        if (defObj.valid() && defObj.is<int>()) def.intVal = defObj.as<int>();
    }
    else if (typeStr == "bool" || typeStr == "boolean")
    {
        def.type = ScriptPropertyType::Bool;
        if (defObj.valid() && defObj.get_type() == sol::type::boolean) def.boolVal = defObj.as<bool>();
    }
    else if (typeStr == "string")
    {
        def.type = ScriptPropertyType::String;
        if (defObj.valid() && defObj.is<std::string>()) def.stringVal = defObj.as<std::string>();
    }
    else if (typeStr == "entity" || typeStr == "entityhandle")
    {
        def.type = ScriptPropertyType::Entity;
        if (defObj.valid())
        {
            try
            {
                if (defObj.is<std::string>())
                    def.entityVal.uuid = std::stoull(defObj.as<std::string>());
                else if (defObj.is<uint64_t>())
                    def.entityVal.uuid = defObj.as<uint64_t>();
                else if (defObj.is<int64_t>() && defObj.as<int64_t>() > 0)
                    def.entityVal.uuid = static_cast<EntityUUID>(defObj.as<int64_t>());
            }
            catch (const std::exception&) {}
        }
    }
    else if (typeStr == "vec2")
    {
        def.type = ScriptPropertyType::Vec2;
        if (defObj.valid())
        {
            if (defObj.is<glm::vec2>()) def.vec2Val = defObj.as<glm::vec2>();
            else if (defObj.is<glm::vec2*>()) { auto* p = defObj.as<glm::vec2*>(); if (p) def.vec2Val = *p; }
            else if (defObj.is<sol::table>()) {
                sol::table t = defObj.as<sol::table>();
                if (t[1].valid() && t[2].valid()) def.vec2Val = glm::vec2(t[1].get<float>(), t[2].get<float>());
            }
            else {
                sol::state_view sv(defObj.lua_state());
                sol::function toStr = sv["tostring"];
                if (toStr.valid()) {
                    std::string s = toStr(defObj);
                    float x = 0, y = 0;
                    if (sscanf(s.c_str(), "Vec2(%f, %f)", &x, &y) >= 2) def.vec2Val = glm::vec2(x, y);
                }
            }
        }
    }
    else if (typeStr == "vec3")
    {
        def.type = ScriptPropertyType::Vec3;
        if (defObj.valid())
        {
            if (defObj.is<glm::vec3>()) def.vec3Val = defObj.as<glm::vec3>();
            else if (defObj.is<glm::vec3*>()) { auto* p = defObj.as<glm::vec3*>(); if (p) def.vec3Val = *p; }
            else if (defObj.is<sol::table>()) {
                sol::table t = defObj.as<sol::table>();
                if (t[1].valid() && t[2].valid() && t[3].valid()) def.vec3Val = glm::vec3(t[1].get<float>(), t[2].get<float>(), t[3].get<float>());
            }
            else {
                sol::state_view sv(defObj.lua_state());
                sol::function toStr = sv["tostring"];
                if (toStr.valid()) {
                    std::string s = toStr(defObj);
                    float x = 0, y = 0, z = 0;
                    if (sscanf(s.c_str(), "Vec3(%f, %f, %f)", &x, &y, &z) >= 3) def.vec3Val = glm::vec3(x, y, z);
                }
            }
        }
    }
    else if (typeStr == "vec4")
    {
        def.type = ScriptPropertyType::Vec4;
        if (defObj.valid())
        {
            if (defObj.is<glm::vec4>()) def.vec4Val = defObj.as<glm::vec4>();
            else if (defObj.is<glm::vec4*>()) { auto* p = defObj.as<glm::vec4*>(); if (p) def.vec4Val = *p; }
            else if (defObj.is<sol::table>()) {
                sol::table t = defObj.as<sol::table>();
                if (t[1].valid() && t[2].valid() && t[3].valid() && t[4].valid()) def.vec4Val = glm::vec4(t[1].get<float>(), t[2].get<float>(), t[3].get<float>(), t[4].get<float>());
            }
            else {
                sol::state_view sv(defObj.lua_state());
                sol::function toStr = sv["tostring"];
                if (toStr.valid()) {
                    std::string s = toStr(defObj);
                    float x = 0, y = 0, z = 0, w = 0;
                    if (sscanf(s.c_str(), "Vec4(%f, %f, %f, %f)", &x, &y, &z, &w) >= 4) def.vec4Val = glm::vec4(x, y, z, w);
                }
            }
        }
    }
    else if (typeStr == "color")
    {
        def.type = ScriptPropertyType::Color;
        if (defObj.valid())
        {
            if (defObj.is<glm::vec4>()) def.vec4Val = defObj.as<glm::vec4>();
            else if (defObj.is<glm::vec4*>()) { auto* p = defObj.as<glm::vec4*>(); if (p) def.vec4Val = *p; }
            else if (defObj.is<sol::table>()) {
                sol::table t = defObj.as<sol::table>();
                if (t[1].valid() && t[2].valid() && t[3].valid() && t[4].valid()) def.vec4Val = glm::vec4(t[1].get<float>(), t[2].get<float>(), t[3].get<float>(), t[4].get<float>());
            }
            else {
                sol::state_view sv(defObj.lua_state());
                sol::function toStr = sv["tostring"];
                if (toStr.valid()) {
                    std::string s = toStr(defObj);
                    float x = 0, y = 0, z = 0, w = 0;
                    if (sscanf(s.c_str(), "Vec4(%f, %f, %f, %f)", &x, &y, &z, &w) >= 4) def.vec4Val = glm::vec4(x, y, z, w);
                }
            }
        }
    }
    else
    {
        if (defObj.valid())
        {
            InferPropertyFromValue(def, defObj);
        }
    }
}

std::vector<ScriptPropertyDef> ScriptEngine::ExtractProperties(const std::string& scriptSource)
{
    std::vector<ScriptPropertyDef> properties;
    if (scriptSource.empty())
        return properties;

    sol::state tempLua;
    tempLua.set_panic(sol::c_call<decltype(&SafeLuaPanic), &SafeLuaPanic>);
    tempLua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

    LuaBindings::RegisterMath(tempLua);
    LuaBindings::RegisterCoreTypes(tempLua);
    LuaBindings::RegisterInputAPI(tempLua);

    tempLua["AssetManager"] = tempLua.create_table();
    tempLua["Assets"] = tempLua["AssetManager"];

    sol::load_result loaded = tempLua.load(scriptSource);
    if (!loaded.valid())
        return properties;

    sol::protected_function func = loaded;
    sol::protected_function_result result = func();

    sol::table propsTable;
    if (result.valid() && result.get_type() == sol::type::table)
    {
        sol::table scriptTable = result;
        sol::optional<sol::table> p = scriptTable["properties"];
        if (p.has_value() && p.value().valid())
            propsTable = p.value();
    }

    if (!propsTable.valid())
    {
        sol::optional<sol::table> p = tempLua["properties"];
        if (p.has_value() && p.value().valid())
            propsTable = p.value();
    }

    if (!propsTable.valid())
        return properties;

    bool isArrayStyle = false;
    for (const auto& [key, val] : propsTable)
    {
        ScriptPropertyDef def;
        if (key.get_type() == sol::type::number && val.get_type() == sol::type::table)
        {
            isArrayStyle = true;
            sol::table item = val.as<sol::table>();
            sol::object nameObj = item["name"];
            if (!nameObj.valid() || !nameObj.is<std::string>())
                continue;

            def.name = nameObj.as<std::string>();
            ParsePropertyTableItem(def, item);
            properties.push_back(def);
        }
        else if (key.is<std::string>())
        {
            def.name = key.as<std::string>();
            if (val.get_type() == sol::type::table)
            {
                sol::table item = val.as<sol::table>();
                ParsePropertyTableItem(def, item);
            }
            else
            {
                InferPropertyFromValue(def, val);
            }
            properties.push_back(def);
        }
    }

    if (!isArrayStyle)
    {
        std::sort(properties.begin(), properties.end(), [](const ScriptPropertyDef& a, const ScriptPropertyDef& b) {
            return a.name < b.name;
        });
    }

    return properties;
}

const std::vector<ScriptPropertyDef>& ScriptEngine::GetScriptPropertyDefs(const AssetUUID& scriptUUID)
{
    auto it = s_PropertyCache.find(scriptUUID);
    if (it != s_PropertyCache.end())
        return it->second;

    auto scriptAsset = AssetManager::Get().GetAsset<ScriptAsset>(scriptUUID);
    if (scriptAsset && scriptAsset->IsValid())
    {
        s_PropertyCache[scriptUUID] = ExtractProperties(scriptAsset->GetSource());
    }
    else
    {
        s_PropertyCache[scriptUUID] = {};
    }
    return s_PropertyCache[scriptUUID];
}

void ScriptEngine::InvalidateScriptPropertyDefs(const AssetUUID& scriptUUID)
{
    s_PropertyCache.erase(scriptUUID);
}

void ScriptEngine::ClearScriptPropertyDefs()
{
    s_PropertyCache.clear();
}

