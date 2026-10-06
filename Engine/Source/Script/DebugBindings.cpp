#include "rvelapch.h"
#include "DebugBindings.h"
#include "Core/Log.h"
#include "Renderer/DebugRenderer.h"
#include "sol/sol.hpp"
#include <vector>

using namespace rv;

namespace {
std::vector<sol::object> GetDebugArguments(sol::variadic_args args)
{
    std::vector<sol::object> values;
    for (auto arg : args)
        values.push_back(arg.get<sol::object>());

    if (!values.empty() && values.front().get_type() == sol::type::table)
        values.erase(values.begin());
    return values;
}

bool TryGetDebugVector(const std::vector<sol::object>& args, size_t index, glm::vec3& value)
{
    if (index >= args.size() || !args[index].is<glm::vec3>())
        return false;
    value = args[index].as<glm::vec3>();
    return true;
}

glm::vec4 GetDebugColor(const std::vector<sol::object>& args, size_t index)
{
    if (index < args.size() && args[index].is<glm::vec4>())
        return args[index].as<glm::vec4>();
    return glm::vec4(1.0f);
}

void LogFromLua(sol::variadic_args args, spdlog::level::level_enum level)
{
    std::stringstream message;
    bool first = true;
    for (const auto& value : GetDebugArguments(args))
    {
        if (!first)
            message << '\t';
        sol::state_view lua(value.lua_state());
        std::string text = lua["tostring"](value);
        message << text;
        first = false;
    }

    if (auto& logger = RvelaLog::GetLogger(); logger)
        logger->log(level, "{}", message.str());
}
}

void LuaBindings::RegisterDebugAPI(sol::state& lua)
{
    lua["Debug"] = lua.create_table();
    sol::table debug = lua["Debug"];
    debug["DrawLine"] = [](sol::variadic_args args)
    {
        auto values = GetDebugArguments(args);
        glm::vec3 from, to;
        if (!TryGetDebugVector(values, 0, from) || !TryGetDebugVector(values, 1, to))
        {
            LOG_WARN("Debug:DrawLine expects (Vec3 from, Vec3 to, optional Vec4 color)");
            return;
        }
        DebugRenderer::Get().DrawLine(from, to, GetDebugColor(values, 2));
    };
    debug["DrawTriangle"] = [](sol::variadic_args args)
    {
        auto values = GetDebugArguments(args);
        glm::vec3 a, b, c;
        if (!TryGetDebugVector(values, 0, a) || !TryGetDebugVector(values, 1, b) || !TryGetDebugVector(values, 2, c))
        {
            LOG_WARN("Debug:DrawTriangle expects (Vec3 a, Vec3 b, Vec3 c, optional Vec4 color)");
            return;
        }
        DebugRenderer::Get().DrawTriangle(a, b, c, GetDebugColor(values, 3));
    };
    debug["DrawBox"] = [](sol::variadic_args args)
    {
        auto values = GetDebugArguments(args);
        glm::vec3 min, max;
        if (!TryGetDebugVector(values, 0, min) || !TryGetDebugVector(values, 1, max))
        {
            LOG_WARN("Debug:DrawBox expects (Vec3 min, Vec3 max, optional Vec4 color)");
            return;
        }
        DebugRenderer::Get().DrawBox(min, max, GetDebugColor(values, 2));
    };

    debug["Trace"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::trace); };
    debug["Debug"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::debug); };
    debug["Info"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::info); };
    debug["Warn"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::warn); };
    debug["Error"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::err); };
    debug["Fatal"] = [](sol::variadic_args args) { LogFromLua(args, spdlog::level::critical); };
    debug["LogTrace"] = debug["Trace"];
    debug["LogDebug"] = debug["Debug"];
    debug["Log"] = debug["Info"];
    debug["LogInfo"] = debug["Info"];
    debug["LogWarning"] = debug["Warn"];
    debug["LogError"] = debug["Error"];
    debug["LogFatal"] = debug["Fatal"];
}
