#pragma once
#include "sol/sol.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "entt/entt.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>

namespace rv {

class Scene;

enum class TweenEase
{
    Linear = 0,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InQuint, OutQuint, InOutQuint,
    InSine, OutSine, InOutSine,
    InExpo, OutExpo, InOutExpo,
    InCirc, OutCirc, InOutCirc,
    InElastic, OutElastic, InOutElastic,
    InBack, OutBack, InOutBack,
    InBounce, OutBounce, InOutBounce
};

TweenEase ParseTweenEase(const std::string& name);
const char* TweenEaseToString(TweenEase ease);
float EvaluateTweenEase(TweenEase ease, float t);

enum class TweenValueType
{
    Float,
    Vec2,
    Vec3,
    Vec4,
    Quat
};

struct TweenValue
{
    TweenValueType type = TweenValueType::Float;
    glm::vec4 vec{ 0.0f };
    glm::quat quat{ 1.0f, 0.0f, 0.0f, 0.0f };

    static TweenValue FromFloat(float v)
    {
        TweenValue tv;
        tv.type = TweenValueType::Float;
        tv.vec.x = v;
        return tv;
    }

    static TweenValue FromVec2(const glm::vec2& v)
    {
        TweenValue tv;
        tv.type = TweenValueType::Vec2;
        tv.vec = glm::vec4(v, 0.0f, 0.0f);
        return tv;
    }

    static TweenValue FromVec3(const glm::vec3& v)
    {
        TweenValue tv;
        tv.type = TweenValueType::Vec3;
        tv.vec = glm::vec4(v, 0.0f);
        return tv;
    }

    static TweenValue FromVec4(const glm::vec4& v)
    {
        TweenValue tv;
        tv.type = TweenValueType::Vec4;
        tv.vec = v;
        return tv;
    }

    static TweenValue FromQuat(const glm::quat& q)
    {
        TweenValue tv;
        tv.type = TweenValueType::Quat;
        tv.quat = q;
        return tv;
    }

    static TweenValue FromLua(const sol::object& obj);
    sol::object ToLua(lua_State* L) const;
    static TweenValue Interpolate(const TweenValue& a, const TweenValue& b, float t);
};

enum class TweenType
{
    Value,
    Position,
    RotationEuler,
    RotationQuat,
    Scale,
    TableProperties
};

struct TweenPropertyTrack
{
    std::string key;
    TweenValue from;
    TweenValue to;
    bool initialized = false;
};

class TweenInstance : public std::enable_shared_from_this<TweenInstance>
{
public:
    TweenInstance(uint64_t id, Scene* scene, entt::entity ownerEntity);
    ~TweenInstance() = default;

    // Fluent Chaining API
    TweenInstance& SetEase(sol::object easeObj);
    TweenInstance& SetDelay(float delay);
    TweenInstance& SetLoops(int loops, sol::optional<bool> yoyo);
    TweenInstance& SetSpeed(float speed);
    TweenInstance& OnUpdate(sol::protected_function callback);
    TweenInstance& OnComplete(sol::protected_function callback);
    TweenInstance& OnLoop(sol::protected_function callback);

    // Control API
    void Pause();
    void Resume();
    void Cancel();
    void Kill() { Cancel(); }
    void Complete();

    bool IsPlaying() const;
    bool IsPaused() const { return m_Paused; }
    float GetProgress() const;
    uint64_t GetId() const { return m_Id; }
    entt::entity GetOwnerEntity() const { return m_OwnerEntity; }

    bool Step(float dt);

    // Configuration Setup
    void SetupValue(const TweenValue& from, const TweenValue& to, float duration, sol::protected_function onUpdate);
    void SetupPosition(entt::entity target, const glm::vec3& toPos, float duration);
    void SetupRotationEuler(entt::entity target, const glm::vec3& toEuler, float duration);
    void SetupRotationQuat(entt::entity target, const glm::quat& toQuat, float duration);
    void SetupScale(entt::entity target, const glm::vec3& toScale, float duration);
    void SetupTable(sol::table targetTable, const std::vector<TweenPropertyTrack>& tracks, float duration);

private:
    void InitializeTracks();
    void ApplyCurrentValue(float easedT);

    uint64_t m_Id = 0;
    Scene* m_Scene = nullptr;
    entt::entity m_OwnerEntity = entt::null;
    TweenType m_Type = TweenType::Value;

    TweenValue m_From;
    TweenValue m_To;
    std::vector<TweenPropertyTrack> m_Tracks;
    sol::table m_TargetTable;

    float m_Duration = 0.0f;
    float m_Elapsed = 0.0f;
    float m_Delay = 0.0f;
    float m_DelayRemaining = 0.0f;
    float m_Speed = 1.0f;

    int m_Loops = 1;
    bool m_Yoyo = false;
    bool m_YoyoBack = false;
    int m_CompletedLoops = 0;

    TweenEase m_Ease = TweenEase::OutQuad;
    bool m_Paused = false;
    bool m_Killed = false;
    bool m_Completed = false;
    bool m_Initialized = false;

    sol::protected_function m_OnUpdate;
    sol::protected_function m_OnComplete;
    sol::protected_function m_OnLoop;
};

class TweenManager
{
public:
    TweenManager();
    ~TweenManager();

    static TweenManager& Get();

    void SetScene(Scene* scene) { m_Scene = scene; }
    Scene* GetScene() const { return m_Scene; }

    void Update(float dt);
    void Cancel(uint64_t id);
    void CancelEntity(entt::entity entity);
    void CancelAll();
    void PauseAll();
    void ResumeAll();
    bool IsPlaying(uint64_t id) const;

    std::shared_ptr<TweenInstance> CreateValueTween(entt::entity owner, const TweenValue& from, const TweenValue& to, float duration, sol::protected_function onUpdate);
    std::shared_ptr<TweenInstance> CreatePositionTween(entt::entity target, const glm::vec3& toPos, float duration);
    std::shared_ptr<TweenInstance> CreateRotationTween(entt::entity target, sol::object toRot, float duration);
    std::shared_ptr<TweenInstance> CreateScaleTween(entt::entity target, const glm::vec3& toScale, float duration);
    std::shared_ptr<TweenInstance> CreateTableTween(entt::entity owner, sol::table targetTable, sol::table propsTable, float duration);

private:
    std::shared_ptr<TweenInstance> AllocateTween(entt::entity owner);

    static TweenManager* s_ActiveInstance;

    uint64_t m_NextId = 1;
    Scene* m_Scene = nullptr;
    std::vector<std::shared_ptr<TweenInstance>> m_Tweens;
    std::vector<std::shared_ptr<TweenInstance>> m_PendingTweens;
    bool m_IsUpdating = false;
};

}
