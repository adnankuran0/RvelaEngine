#include "rvelapch.h"
#include "MathBindings.h"
#include "sol/sol.hpp"
#include <format>

using namespace rv;

void LuaBindings::RegisterMath(sol::state& lua)
{
    // Vec2
    lua.new_usertype<glm::vec2>("Vec2",
        sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,

        "Length", [](const glm::vec2& v) { return glm::length(v); },
        "LengthSq", [](const glm::vec2& v) { return glm::dot(v, v); },
        "Normalized", [](const glm::vec2& v) { return glm::normalize(v); },

        "Dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
        "Distance", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); },
        "Lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return glm::mix(a, b, t); },
        "__add", [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        "__sub", [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        "__mul", sol::overload(
            [](const glm::vec2& a, float s) { return a * s; },
            [](float s, const glm::vec2& a) { return a * s; }
        ),
        "__div", [](const glm::vec2& a, float s) { return a / s; },
        sol::meta_function::to_string, [](const glm::vec2& v)
        {
            return std::format("Vec2({}, {})", v.x, v.y);
        }
    );
    lua["Vec2"]["ZERO"] = []() { return glm::vec2(0.0f); };
    lua["Vec2"]["UP"] = []() { return glm::vec2(0.0f, 1.0f); };
    lua["Vec2"]["RIGHT"] = []() { return glm::vec2(1.0f, 0.0f); };

    // Vec3
    lua.new_usertype<glm::vec3>("Vec3",
        sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,

        "Length", [](const glm::vec3& v) { return glm::length(v); },
        "LengthSq", [](const glm::vec3& v) { return glm::dot(v, v); },
        "Normalized", [](const glm::vec3& v) { return glm::normalize(v); },

        "Dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "Cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
        "Distance", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); },
        "Lerp", [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); },

        "Reflect", [](const glm::vec3& v, const glm::vec3& n) { return glm::reflect(v, n); },

        "__add", [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        "__sub", [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        "__mul", sol::overload(
            [](const glm::vec3& a, float s) { return a * s; },
            [](float s, const glm::vec3& a) { return a * s; }
        ),
        "__div", [](const glm::vec3& a, float s) { return a / s; },
        sol::meta_function::to_string, [](const glm::vec3& v)
        {
            return std::format("Vec3({}, {}, {})", v.x, v.y, v.z);
        }
    );
    lua["Vec3"]["ZERO"] = []() { return glm::vec3(0.0f); };
    lua["Vec3"]["UP"] = []() { return glm::vec3(0.0f, 1.0f, 0.0f); };
    lua["Vec3"]["RIGHT"] = []() { return glm::vec3(1.0f, 0.0f, 0.0f); };
    lua["Vec3"]["FORWARD"] = []() { return glm::vec3(0.0f, 0.0f, -1.0f); };

    // Vec4
    lua.new_usertype<glm::vec4>("Vec4",
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x,
        "y", &glm::vec4::y,
        "z", &glm::vec4::z,
        "w", &glm::vec4::w,
        "r", &glm::vec4::r,
        "g", &glm::vec4::g,
        "b", &glm::vec4::b,
        "a", &glm::vec4::a,

        "Length", [](const glm::vec4& v) { return glm::length(v); },
        "LengthSq", [](const glm::vec4& v) { return glm::dot(v, v); },
        "Normalized", [](const glm::vec4& v) { return glm::normalize(v); },

        "Dot", [](const glm::vec4& a, const glm::vec4& b) { return glm::dot(a, b); },
        "Distance", [](const glm::vec4& a, const glm::vec4& b) { return glm::distance(a, b); },
        "Lerp", [](const glm::vec4& a, const glm::vec4& b, float t) { return glm::mix(a, b, t); },

        "__add", [](const glm::vec4& a, const glm::vec4& b) { return a + b; },
        "__sub", [](const glm::vec4& a, const glm::vec4& b) { return a - b; },
        "__mul", sol::overload(
            [](const glm::vec4& a, float s) { return a * s; },
            [](float s, const glm::vec4& a) { return a * s; },
            [](const glm::vec4& a, const glm::vec4& b) { return a * b; }
        ),
        "__div", [](const glm::vec4& a, float s) { return a / s; },
        sol::meta_function::to_string, [](const glm::vec4& v)
        {
            return std::format("Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
        }
    );
    lua["Vec4"]["ZERO"] = []() { return glm::vec4(0.0f); };
    lua["Vec4"]["ONE"] = []() { return glm::vec4(1.0f); };

    // Quat
    lua.new_usertype<glm::quat>("Quat",
        sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),

        "Normalized", [](const glm::quat& q) { return glm::normalize(q); },
        "ToEuler", [](const glm::quat& q) { return glm::eulerAngles(q); },
        "Rotate", [](const glm::quat& q, const glm::vec3& v) { return q * v; },

        "__mul", sol::overload(
            [](const glm::quat& a, const glm::quat& b) { return a * b; },
            [](const glm::quat& q, const glm::vec3& v) { return q * v; }
        )
    );

    // Math Table
    lua["Math"] = lua.create_table();
    lua["Math"]["Clamp"] = [](float v, float min, float max) { return glm::clamp(v, min, max); };
    lua["Math"]["Lerp"] = [](float a, float b, float t) { return glm::mix(a, b, t); };
    lua["Math"]["Radians"] = [](float d) { return glm::radians(d); };
    lua["Math"]["Degrees"] = [](float r) { return glm::degrees(r); };
}