#include "rvelapch.h"
#include "Tween.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components/TransformComponent.h"
#include "Core/Log.h"
#include <cmath>
#include <algorithm>

namespace rv {

TweenManager* TweenManager::s_ActiveInstance = nullptr;

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float HALF_PI = PI * 0.5f;

    std::string ToLowerString(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return str;
    }
}

TweenEase ParseTweenEase(const std::string& name)
{
    std::string s = ToLowerString(name);
    s.erase(std::remove(s.begin(), s.end(), '_'), s.end());

    if (s == "linear") return TweenEase::Linear;
    if (s == "inquad") return TweenEase::InQuad;
    if (s == "outquad") return TweenEase::OutQuad;
    if (s == "inoutquad") return TweenEase::InOutQuad;
    if (s == "incubic") return TweenEase::InCubic;
    if (s == "outcubic") return TweenEase::OutCubic;
    if (s == "inoutcubic") return TweenEase::InOutCubic;
    if (s == "inquart") return TweenEase::InQuart;
    if (s == "outquart") return TweenEase::OutQuart;
    if (s == "inoutquart") return TweenEase::InOutQuart;
    if (s == "inquint") return TweenEase::InQuint;
    if (s == "outquint") return TweenEase::OutQuint;
    if (s == "inoutquint") return TweenEase::InOutQuint;
    if (s == "insine") return TweenEase::InSine;
    if (s == "outsine") return TweenEase::OutSine;
    if (s == "inoutsine") return TweenEase::InOutSine;
    if (s == "inexpo") return TweenEase::InExpo;
    if (s == "outexpo") return TweenEase::OutExpo;
    if (s == "inoutexpo") return TweenEase::InOutExpo;
    if (s == "incirc") return TweenEase::InCirc;
    if (s == "outcirc") return TweenEase::OutCirc;
    if (s == "inoutcirc") return TweenEase::InOutCirc;
    if (s == "inelastic") return TweenEase::InElastic;
    if (s == "outelastic") return TweenEase::OutElastic;
    if (s == "inoutelastic") return TweenEase::InOutElastic;
    if (s == "inback") return TweenEase::InBack;
    if (s == "outback") return TweenEase::OutBack;
    if (s == "inoutback") return TweenEase::InOutBack;
    if (s == "inbounce") return TweenEase::InBounce;
    if (s == "outbounce") return TweenEase::OutBounce;
    if (s == "inoutbounce") return TweenEase::InOutBounce;

    return TweenEase::OutQuad;
}

const char* TweenEaseToString(TweenEase ease)
{
    switch (ease)
    {
    case TweenEase::Linear: return "Linear";
    case TweenEase::InQuad: return "InQuad";
    case TweenEase::OutQuad: return "OutQuad";
    case TweenEase::InOutQuad: return "InOutQuad";
    case TweenEase::InCubic: return "InCubic";
    case TweenEase::OutCubic: return "OutCubic";
    case TweenEase::InOutCubic: return "InOutCubic";
    case TweenEase::InQuart: return "InQuart";
    case TweenEase::OutQuart: return "OutQuart";
    case TweenEase::InOutQuart: return "InOutQuart";
    case TweenEase::InQuint: return "InQuint";
    case TweenEase::OutQuint: return "OutQuint";
    case TweenEase::InOutQuint: return "InOutQuint";
    case TweenEase::InSine: return "InSine";
    case TweenEase::OutSine: return "OutSine";
    case TweenEase::InOutSine: return "InOutSine";
    case TweenEase::InExpo: return "InExpo";
    case TweenEase::OutExpo: return "OutExpo";
    case TweenEase::InOutExpo: return "InOutExpo";
    case TweenEase::InCirc: return "InCirc";
    case TweenEase::OutCirc: return "OutCirc";
    case TweenEase::InOutCirc: return "InOutCirc";
    case TweenEase::InElastic: return "InElastic";
    case TweenEase::OutElastic: return "OutElastic";
    case TweenEase::InOutElastic: return "InOutElastic";
    case TweenEase::InBack: return "InBack";
    case TweenEase::OutBack: return "OutBack";
    case TweenEase::InOutBack: return "InOutBack";
    case TweenEase::InBounce: return "InBounce";
    case TweenEase::OutBounce: return "OutBounce";
    case TweenEase::InOutBounce: return "InOutBounce";
    default: return "OutQuad";
    }
}

static float EvaluateOutBounce(float t)
{
    if (t < (1.0f / 2.75f))
    {
        return 7.5625f * t * t;
    }
    else if (t < (2.0f / 2.75f))
    {
        t -= (1.5f / 2.75f);
        return 7.5625f * t * t + 0.75f;
    }
    else if (t < (2.5f / 2.75f))
    {
        t -= (2.25f / 2.75f);
        return 7.5625f * t * t + 0.9375f;
    }
    else
    {
        t -= (2.625f / 2.75f);
        return 7.5625f * t * t + 0.984375f;
    }
}

float EvaluateTweenEase(TweenEase ease, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    switch (ease)
    {
    case TweenEase::Linear:
        return t;

    case TweenEase::InQuad:
        return t * t;
    case TweenEase::OutQuad:
        return t * (2.0f - t);
    case TweenEase::InOutQuad:
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

    case TweenEase::InCubic:
        return t * t * t;
    case TweenEase::OutCubic: {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    case TweenEase::InOutCubic: {
        if (t < 0.5f) return 4.0f * t * t * t;
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }

    case TweenEase::InQuart:
        return t * t * t * t;
    case TweenEase::OutQuart: {
        float f = t - 1.0f;
        return 1.0f - f * f * f * f;
    }
    case TweenEase::InOutQuart: {
        if (t < 0.5f) return 8.0f * t * t * t * t;
        float f = t - 1.0f;
        return 1.0f - 8.0f * f * f * f * f;
    }

    case TweenEase::InQuint:
        return t * t * t * t * t;
    case TweenEase::OutQuint: {
        float f = t - 1.0f;
        return f * f * f * f * f + 1.0f;
    }
    case TweenEase::InOutQuint: {
        if (t < 0.5f) return 16.0f * t * t * t * t * t;
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f * f * f + 1.0f;
    }

    case TweenEase::InSine:
        return 1.0f - std::cos(t * HALF_PI);
    case TweenEase::OutSine:
        return std::sin(t * HALF_PI);
    case TweenEase::InOutSine:
        return 0.5f * (1.0f - std::cos(PI * t));

    case TweenEase::InExpo:
        return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
    case TweenEase::OutExpo:
        return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    case TweenEase::InOutExpo:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        if (t < 0.5f) return 0.5f * std::pow(2.0f, (20.0f * t) - 10.0f);
        return 1.0f - 0.5f * std::pow(2.0f, (-20.0f * t) + 10.0f);

    case TweenEase::InCirc:
        return 1.0f - std::sqrt(std::max(0.0f, 1.0f - t * t));
    case TweenEase::OutCirc:
        return std::sqrt(std::max(0.0f, 1.0f - (t - 1.0f) * (t - 1.0f)));
    case TweenEase::InOutCirc:
        if (t < 0.5f)
            return 0.5f * (1.0f - std::sqrt(std::max(0.0f, 1.0f - 4.0f * t * t)));
        return 0.5f * (std::sqrt(std::max(0.0f, 1.0f - (2.0f * t - 2.0f) * (2.0f * t - 2.0f))) + 1.0f);

    case TweenEase::InElastic:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * PI);
    case TweenEase::OutElastic:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * 5.0f * PI) + 1.0f;
    case TweenEase::InOutElastic:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        t *= 2.0f;
        if (t < 1.0f)
            return -0.5f * std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * PI);
        return 0.5f * std::pow(2.0f, -10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * PI) + 1.0f;

    case TweenEase::InBack: {
        constexpr float s = 1.70158f;
        return t * t * ((s + 1.0f) * t - s);
    }
    case TweenEase::OutBack: {
        constexpr float s = 1.70158f;
        float f = t - 1.0f;
        return f * f * ((s + 1.0f) * f + s) + 1.0f;
    }
    case TweenEase::InOutBack: {
        constexpr float s = 1.70158f * 1.525f;
        t *= 2.0f;
        if (t < 1.0f)
            return 0.5f * (t * t * ((s + 1.0f) * t - s));
        t -= 2.0f;
        return 0.5f * (t * t * ((s + 1.0f) * t + s) + 2.0f);
    }

    case TweenEase::OutBounce:
        return EvaluateOutBounce(t);
    case TweenEase::InBounce:
        return 1.0f - EvaluateOutBounce(1.0f - t);
    case TweenEase::InOutBounce:
        if (t < 0.5f)
            return 0.5f * (1.0f - EvaluateOutBounce(1.0f - (t * 2.0f)));
        return 0.5f * EvaluateOutBounce(t * 2.0f - 1.0f) + 0.5f;

    default:
        return t;
    }
}

TweenValue TweenValue::FromLua(const sol::object& obj)
{
    if (!obj.valid())
        return TweenValue::FromFloat(0.0f);

    if (obj.is<float>() || obj.is<double>() || obj.is<int>())
    {
        return TweenValue::FromFloat(obj.as<float>());
    }
    if (obj.is<glm::vec2>())
    {
        return TweenValue::FromVec2(obj.as<glm::vec2>());
    }
    if (obj.is<glm::vec2*>())
    {
        auto* p = obj.as<glm::vec2*>();
        return TweenValue::FromVec2(p ? *p : glm::vec2(0.0f));
    }
    if (obj.is<glm::vec3>())
    {
        return TweenValue::FromVec3(obj.as<glm::vec3>());
    }
    if (obj.is<glm::vec3*>())
    {
        auto* p = obj.as<glm::vec3*>();
        return TweenValue::FromVec3(p ? *p : glm::vec3(0.0f));
    }
    if (obj.is<glm::vec4>())
    {
        return TweenValue::FromVec4(obj.as<glm::vec4>());
    }
    if (obj.is<glm::vec4*>())
    {
        auto* p = obj.as<glm::vec4*>();
        return TweenValue::FromVec4(p ? *p : glm::vec4(0.0f));
    }
    if (obj.is<glm::quat>())
    {
        return TweenValue::FromQuat(obj.as<glm::quat>());
    }
    if (obj.is<glm::quat*>())
    {
        auto* p = obj.as<glm::quat*>();
        return TweenValue::FromQuat(p ? *p : glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    }

    return TweenValue::FromFloat(0.0f);
}

sol::object TweenValue::ToLua(lua_State* L) const
{
    switch (type)
    {
    case TweenValueType::Float:
        return sol::make_object(L, vec.x);
    case TweenValueType::Vec2:
        return sol::make_object(L, glm::vec2(vec.x, vec.y));
    case TweenValueType::Vec3:
        return sol::make_object(L, glm::vec3(vec.x, vec.y, vec.z));
    case TweenValueType::Vec4:
        return sol::make_object(L, vec);
    case TweenValueType::Quat:
        return sol::make_object(L, quat);
    default:
        return sol::make_object(L, vec.x);
    }
}

TweenValue TweenValue::Interpolate(const TweenValue& a, const TweenValue& b, float t)
{
    TweenValue res;
    res.type = b.type;

    switch (b.type)
    {
    case TweenValueType::Float:
        res.vec.x = a.vec.x + (b.vec.x - a.vec.x) * t;
        break;
    case TweenValueType::Vec2: {
        glm::vec2 av(a.vec.x, a.vec.y);
        glm::vec2 bv(b.vec.x, b.vec.y);
        glm::vec2 mixVal = glm::mix(av, bv, t);
        res.vec = glm::vec4(mixVal, 0.0f, 0.0f);
        break;
    }
    case TweenValueType::Vec3: {
        glm::vec3 av(a.vec.x, a.vec.y, a.vec.z);
        glm::vec3 bv(b.vec.x, b.vec.y, b.vec.z);
        glm::vec3 mixVal = glm::mix(av, bv, t);
        res.vec = glm::vec4(mixVal, 0.0f);
        break;
    }
    case TweenValueType::Vec4:
        res.vec = glm::mix(a.vec, b.vec, t);
        break;
    case TweenValueType::Quat:
        res.quat = glm::slerp(a.quat, b.quat, t);
        break;
    }

    return res;
}

TweenInstance::TweenInstance(uint64_t id, Scene* scene, entt::entity ownerEntity)
    : m_Id(id), m_Scene(scene), m_OwnerEntity(ownerEntity)
{
}

TweenInstance& TweenInstance::SetEase(sol::object easeObj)
{
    if (easeObj.is<int>())
    {
        m_Ease = static_cast<TweenEase>(easeObj.as<int>());
    }
    else if (easeObj.is<std::string>())
    {
        m_Ease = ParseTweenEase(easeObj.as<std::string>());
    }
    return *this;
}

TweenInstance& TweenInstance::SetDelay(float delay)
{
    m_Delay = std::max(0.0f, delay);
    m_DelayRemaining = m_Delay;
    return *this;
}

TweenInstance& TweenInstance::SetLoops(int loops, sol::optional<bool> yoyo)
{
    m_Loops = loops;
    if (yoyo.has_value())
        m_Yoyo = yoyo.value();
    return *this;
}

TweenInstance& TweenInstance::SetSpeed(float speed)
{
    m_Speed = std::max(0.0f, speed);
    return *this;
}

TweenInstance& TweenInstance::OnUpdate(sol::protected_function callback)
{
    m_OnUpdate = callback;
    return *this;
}

TweenInstance& TweenInstance::OnComplete(sol::protected_function callback)
{
    m_OnComplete = callback;
    return *this;
}

TweenInstance& TweenInstance::OnLoop(sol::protected_function callback)
{
    m_OnLoop = callback;
    return *this;
}

void TweenInstance::Pause()
{
    m_Paused = true;
}

void TweenInstance::Resume()
{
    m_Paused = false;
}

void TweenInstance::Cancel()
{
    m_Killed = true;
}

void TweenInstance::Complete()
{
    if (m_Killed || m_Completed)
        return;

    InitializeTracks();
    ApplyCurrentValue(m_Yoyo && m_YoyoBack ? 0.0f : 1.0f);

    m_Completed = true;
    m_Killed = true;

    if (m_OnComplete.valid())
    {
        sol::protected_function_result res = m_OnComplete();
        if (!res.valid())
        {
            sol::error err = res;
            LOG_ERROR("Lua Tween OnComplete error: {}", err.what());
        }
    }
}

bool TweenInstance::IsPlaying() const
{
    return !m_Paused && !m_Killed && !m_Completed;
}

float TweenInstance::GetProgress() const
{
    if (m_Duration <= 0.0f) return 1.0f;
    return std::clamp(m_Elapsed / m_Duration, 0.0f, 1.0f);
}

void TweenInstance::SetupValue(const TweenValue& from, const TweenValue& to, float duration, sol::protected_function onUpdate)
{
    m_Type = TweenType::Value;
    m_From = from;
    m_To = to;
    m_Duration = std::max(0.0001f, duration);
    m_OnUpdate = onUpdate;
    m_Initialized = true;
}

void TweenInstance::SetupPosition(entt::entity target, const glm::vec3& toPos, float duration)
{
    m_Type = TweenType::Position;
    m_OwnerEntity = target;
    m_To = TweenValue::FromVec3(toPos);
    m_Duration = std::max(0.0001f, duration);
}

void TweenInstance::SetupRotationEuler(entt::entity target, const glm::vec3& toEuler, float duration)
{
    m_Type = TweenType::RotationEuler;
    m_OwnerEntity = target;
    m_To = TweenValue::FromVec3(toEuler);
    m_Duration = std::max(0.0001f, duration);
}

void TweenInstance::SetupRotationQuat(entt::entity target, const glm::quat& toQuat, float duration)
{
    m_Type = TweenType::RotationQuat;
    m_OwnerEntity = target;
    m_To = TweenValue::FromQuat(toQuat);
    m_Duration = std::max(0.0001f, duration);
}

void TweenInstance::SetupScale(entt::entity target, const glm::vec3& toScale, float duration)
{
    m_Type = TweenType::Scale;
    m_OwnerEntity = target;
    m_To = TweenValue::FromVec3(toScale);
    m_Duration = std::max(0.0001f, duration);
}

void TweenInstance::SetupTable(sol::table targetTable, const std::vector<TweenPropertyTrack>& tracks, float duration)
{
    m_Type = TweenType::TableProperties;
    m_TargetTable = targetTable;
    m_Tracks = tracks;
    m_Duration = std::max(0.0001f, duration);
}

void TweenInstance::InitializeTracks()
{
    if (m_Initialized)
        return;

    m_Initialized = true;

    if (m_Type == TweenType::Position)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                m_From = TweenValue::FromVec3(tc->GetPosition());
            }
        }
    }
    else if (m_Type == TweenType::RotationEuler)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                m_From = TweenValue::FromVec3(tc->GetEulerRotation());
            }
        }
    }
    else if (m_Type == TweenType::RotationQuat)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                m_From = TweenValue::FromQuat(tc->GetRotation());
            }
        }
    }
    else if (m_Type == TweenType::Scale)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                m_From = TweenValue::FromVec3(tc->GetScale());
            }
        }
    }
    else if (m_Type == TweenType::TableProperties)
    {
        if (m_TargetTable.valid())
        {
            for (auto& track : m_Tracks)
            {
                if (!track.initialized)
                {
                    sol::object curVal = m_TargetTable[track.key];
                    track.from = TweenValue::FromLua(curVal);
                    track.initialized = true;
                }
            }
        }
    }
}

void TweenInstance::ApplyCurrentValue(float easedT)
{
    if (m_Type == TweenType::Value)
    {
        TweenValue current = TweenValue::Interpolate(m_From, m_To, easedT);
        if (m_OnUpdate.valid())
        {
            sol::object luaVal = current.ToLua(m_OnUpdate.lua_state());
            sol::protected_function_result res = m_OnUpdate(luaVal);
            if (!res.valid())
            {
                sol::error err = res;
                LOG_ERROR("Lua Tween OnUpdate error: {}", err.what());
            }
        }
    }
    else if (m_Type == TweenType::Position)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                TweenValue current = TweenValue::Interpolate(m_From, m_To, easedT);
                glm::vec3 pos(current.vec.x, current.vec.y, current.vec.z);
                tc->SetPosition(pos);

                if (m_OnUpdate.valid())
                {
                    sol::object luaVal = current.ToLua(m_OnUpdate.lua_state());
                    m_OnUpdate(luaVal);
                }
            }
        }
    }
    else if (m_Type == TweenType::RotationEuler)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                TweenValue current = TweenValue::Interpolate(m_From, m_To, easedT);
                glm::vec3 euler(current.vec.x, current.vec.y, current.vec.z);
                tc->SetEulerRotation(euler);

                if (m_OnUpdate.valid())
                {
                    sol::object luaVal = current.ToLua(m_OnUpdate.lua_state());
                    m_OnUpdate(luaVal);
                }
            }
        }
    }
    else if (m_Type == TweenType::RotationQuat)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                TweenValue current = TweenValue::Interpolate(m_From, m_To, easedT);
                tc->SetRotation(current.quat);

                if (m_OnUpdate.valid())
                {
                    sol::object luaVal = current.ToLua(m_OnUpdate.lua_state());
                    m_OnUpdate(luaVal);
                }
            }
        }
    }
    else if (m_Type == TweenType::Scale)
    {
        if (m_Scene && m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            if (auto* tc = m_Scene->GetRegistry().try_get<TransformComponent>(m_OwnerEntity))
            {
                TweenValue current = TweenValue::Interpolate(m_From, m_To, easedT);
                glm::vec3 scale(current.vec.x, current.vec.y, current.vec.z);
                tc->SetScale(scale);

                if (m_OnUpdate.valid())
                {
                    sol::object luaVal = current.ToLua(m_OnUpdate.lua_state());
                    m_OnUpdate(luaVal);
                }
            }
        }
    }
    else if (m_Type == TweenType::TableProperties)
    {
        if (m_TargetTable.valid())
        {
            lua_State* L = m_TargetTable.lua_state();
            for (const auto& track : m_Tracks)
            {
                TweenValue current = TweenValue::Interpolate(track.from, track.to, easedT);
                m_TargetTable[track.key] = current.ToLua(L);
            }

            if (m_OnUpdate.valid())
            {
                m_OnUpdate(m_TargetTable);
            }
        }
    }
}

bool TweenInstance::Step(float dt)
{
    if (m_Killed || m_Completed)
        return false;

    if (m_Paused)
        return true;

    if (m_OwnerEntity != entt::null && m_Scene)
    {
        if (!m_Scene->GetRegistry().valid(m_OwnerEntity))
        {
            m_Killed = true;
            return false;
        }
    }

    if (m_DelayRemaining > 0.0f)
    {
        m_DelayRemaining -= dt * m_Speed;
        if (m_DelayRemaining > 0.0f)
            return true;

        dt = -m_DelayRemaining / (m_Speed > 0.0f ? m_Speed : 1.0f);
        m_DelayRemaining = 0.0f;
    }

    InitializeTracks();

    m_Elapsed += dt * m_Speed;
    float normalizedT = std::clamp(m_Elapsed / m_Duration, 0.0f, 1.0f);

    float evalT = normalizedT;
    if (m_Yoyo && m_YoyoBack)
    {
        evalT = 1.0f - normalizedT;
    }

    float easedT = EvaluateTweenEase(m_Ease, evalT);
    ApplyCurrentValue(easedT);

    if (m_Elapsed >= m_Duration)
    {
        m_CompletedLoops++;

        if (m_OnLoop.valid())
        {
            sol::protected_function_result res = m_OnLoop(m_CompletedLoops);
            if (!res.valid())
            {
                sol::error err = res;
                LOG_ERROR("Lua Tween OnLoop error: {}", err.what());
            }
        }

        const bool infinite = (m_Loops < 0);
        if (infinite || m_CompletedLoops < m_Loops)
        {
            m_Elapsed = 0.0f;
            if (m_Yoyo)
            {
                m_YoyoBack = !m_YoyoBack;
            }
            return true;
        }
        else
        {
            m_Completed = true;
            m_Killed = true;

            if (m_OnComplete.valid())
            {
                sol::protected_function_result res = m_OnComplete();
                if (!res.valid())
                {
                    sol::error err = res;
                    LOG_ERROR("Lua Tween OnComplete error: {}", err.what());
                }
            }
            return false;
        }
    }

    return true;
}

TweenManager::TweenManager()
{
    s_ActiveInstance = this;
}

TweenManager::~TweenManager()
{
    if (s_ActiveInstance == this)
        s_ActiveInstance = nullptr;
}

TweenManager& TweenManager::Get()
{
    if (!s_ActiveInstance)
    {
        static TweenManager fallback;
        return fallback;
    }
    return *s_ActiveInstance;
}

void TweenManager::Update(float dt)
{
    m_IsUpdating = true;

    if (!m_PendingTweens.empty())
    {
        m_Tweens.insert(m_Tweens.end(), m_PendingTweens.begin(), m_PendingTweens.end());
        m_PendingTweens.clear();
    }

    for (auto it = m_Tweens.begin(); it != m_Tweens.end(); )
    {
        auto& tween = *it;
        if (!tween || !tween->Step(dt))
        {
            it = m_Tweens.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_IsUpdating = false;
}

void TweenManager::Cancel(uint64_t id)
{
    for (auto& tween : m_Tweens)
    {
        if (tween && tween->GetId() == id)
            tween->Cancel();
    }
    for (auto& tween : m_PendingTweens)
    {
        if (tween && tween->GetId() == id)
            tween->Cancel();
    }
}

void TweenManager::CancelEntity(entt::entity entity)
{
    if (entity == entt::null)
        return;

    for (auto& tween : m_Tweens)
    {
        if (tween && tween->GetOwnerEntity() == entity)
            tween->Cancel();
    }
    for (auto& tween : m_PendingTweens)
    {
        if (tween && tween->GetOwnerEntity() == entity)
            tween->Cancel();
    }
}

void TweenManager::CancelAll()
{
    for (auto& tween : m_Tweens)
    {
        if (tween)
            tween->Cancel();
    }
    for (auto& tween : m_PendingTweens)
    {
        if (tween)
            tween->Cancel();
    }
    m_Tweens.clear();
    m_PendingTweens.clear();
}

void TweenManager::PauseAll()
{
    for (auto& tween : m_Tweens)
    {
        if (tween)
            tween->Pause();
    }
    for (auto& tween : m_PendingTweens)
    {
        if (tween)
            tween->Pause();
    }
}

void TweenManager::ResumeAll()
{
    for (auto& tween : m_Tweens)
    {
        if (tween)
            tween->Resume();
    }
    for (auto& tween : m_PendingTweens)
    {
        if (tween)
            tween->Resume();
    }
}

bool TweenManager::IsPlaying(uint64_t id) const
{
    for (const auto& tween : m_Tweens)
    {
        if (tween && tween->GetId() == id)
            return tween->IsPlaying();
    }
    for (const auto& tween : m_PendingTweens)
    {
        if (tween && tween->GetId() == id)
            return tween->IsPlaying();
    }
    return false;
}

std::shared_ptr<TweenInstance> TweenManager::AllocateTween(entt::entity owner)
{
    const uint64_t id = m_NextId++;
    auto tween = std::make_shared<TweenInstance>(id, m_Scene, owner);
    if (m_IsUpdating)
    {
        m_PendingTweens.push_back(tween);
    }
    else
    {
        m_Tweens.push_back(tween);
    }
    return tween;
}

std::shared_ptr<TweenInstance> TweenManager::CreateValueTween(entt::entity owner, const TweenValue& from, const TweenValue& to, float duration, sol::protected_function onUpdate)
{
    auto tween = AllocateTween(owner);
    tween->SetupValue(from, to, duration, onUpdate);
    return tween;
}

std::shared_ptr<TweenInstance> TweenManager::CreatePositionTween(entt::entity target, const glm::vec3& toPos, float duration)
{
    auto tween = AllocateTween(target);
    tween->SetupPosition(target, toPos, duration);
    return tween;
}

std::shared_ptr<TweenInstance> TweenManager::CreateRotationTween(entt::entity target, sol::object toRot, float duration)
{
    auto tween = AllocateTween(target);
    if (toRot.is<glm::quat>() || toRot.is<glm::quat*>())
    {
        glm::quat q = toRot.is<glm::quat>() ? toRot.as<glm::quat>() : *toRot.as<glm::quat*>();
        tween->SetupRotationQuat(target, q, duration);
    }
    else
    {
        glm::vec3 euler(0.0f);
        if (toRot.is<glm::vec3>()) euler = toRot.as<glm::vec3>();
        else if (toRot.is<glm::vec3*>()) euler = *toRot.as<glm::vec3*>();
        tween->SetupRotationEuler(target, euler, duration);
    }
    return tween;
}

std::shared_ptr<TweenInstance> TweenManager::CreateScaleTween(entt::entity target, const glm::vec3& toScale, float duration)
{
    auto tween = AllocateTween(target);
    tween->SetupScale(target, toScale, duration);
    return tween;
}

std::shared_ptr<TweenInstance> TweenManager::CreateTableTween(entt::entity owner, sol::table targetTable, sol::table propsTable, float duration)
{
    auto tween = AllocateTween(owner);
    std::vector<TweenPropertyTrack> tracks;

    for (const auto& [k, v] : propsTable)
    {
        if (k.is<std::string>())
        {
            TweenPropertyTrack track;
            track.key = k.as<std::string>();
            track.to = TweenValue::FromLua(v);

            sol::object curVal = targetTable[track.key];
            if (curVal.valid())
            {
                track.from = TweenValue::FromLua(curVal);
                track.initialized = true;
            }
            tracks.push_back(track);
        }
    }

    tween->SetupTable(targetTable, tracks, duration);
    return tween;
}

}
