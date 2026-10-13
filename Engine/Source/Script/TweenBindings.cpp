#include "rvelapch.h"
#include "TweenBindings.h"
#include "Tween.h"
#include "Scene/Entity.h"
#include "ComponentHandle.h"
#include "Scene/Components/TransformComponent.h"

namespace rv::LuaBindings {

static entt::entity ResolveEntityFromLua(sol::object obj)
{
    if (obj.is<Entity>())
    {
        return obj.as<Entity>().GetHandle();
    }
    if (obj.is<Entity*>())
    {
        auto* e = obj.as<Entity*>();
        return e ? e->GetHandle() : entt::null;
    }
    if (obj.is<ComponentHandle<TransformComponent>>())
    {
        auto h = obj.as<ComponentHandle<TransformComponent>>();
        return h.entity.GetHandle();
    }
    if (obj.is<sol::table>())
    {
        sol::table t = obj.as<sol::table>();
        sol::object entObj = t["entity"];
        if (entObj.valid() && entObj.is<Entity>())
        {
            return entObj.as<Entity>().GetHandle();
        }
    }
    return entt::null;
}

void RegisterTweenAPI(sol::state& lua)
{
    sol::table easeTable = lua.create_table();
    easeTable["Linear"] = static_cast<int>(TweenEase::Linear);
    easeTable["InQuad"] = static_cast<int>(TweenEase::InQuad);
    easeTable["OutQuad"] = static_cast<int>(TweenEase::OutQuad);
    easeTable["InOutQuad"] = static_cast<int>(TweenEase::InOutQuad);
    easeTable["InCubic"] = static_cast<int>(TweenEase::InCubic);
    easeTable["OutCubic"] = static_cast<int>(TweenEase::OutCubic);
    easeTable["InOutCubic"] = static_cast<int>(TweenEase::InOutCubic);
    easeTable["InQuart"] = static_cast<int>(TweenEase::InQuart);
    easeTable["OutQuart"] = static_cast<int>(TweenEase::OutQuart);
    easeTable["InOutQuart"] = static_cast<int>(TweenEase::InOutQuart);
    easeTable["InQuint"] = static_cast<int>(TweenEase::InQuint);
    easeTable["OutQuint"] = static_cast<int>(TweenEase::OutQuint);
    easeTable["InOutQuint"] = static_cast<int>(TweenEase::InOutQuint);
    easeTable["InSine"] = static_cast<int>(TweenEase::InSine);
    easeTable["OutSine"] = static_cast<int>(TweenEase::OutSine);
    easeTable["InOutSine"] = static_cast<int>(TweenEase::InOutSine);
    easeTable["InExpo"] = static_cast<int>(TweenEase::InExpo);
    easeTable["OutExpo"] = static_cast<int>(TweenEase::OutExpo);
    easeTable["InOutExpo"] = static_cast<int>(TweenEase::InOutExpo);
    easeTable["InCirc"] = static_cast<int>(TweenEase::InCirc);
    easeTable["OutCirc"] = static_cast<int>(TweenEase::OutCirc);
    easeTable["InOutCirc"] = static_cast<int>(TweenEase::InOutCirc);
    easeTable["InElastic"] = static_cast<int>(TweenEase::InElastic);
    easeTable["OutElastic"] = static_cast<int>(TweenEase::OutElastic);
    easeTable["InOutElastic"] = static_cast<int>(TweenEase::InOutElastic);
    easeTable["InBack"] = static_cast<int>(TweenEase::InBack);
    easeTable["OutBack"] = static_cast<int>(TweenEase::OutBack);
    easeTable["InOutBack"] = static_cast<int>(TweenEase::InOutBack);
    easeTable["InBounce"] = static_cast<int>(TweenEase::InBounce);
    easeTable["OutBounce"] = static_cast<int>(TweenEase::OutBounce);
    easeTable["InOutBounce"] = static_cast<int>(TweenEase::InOutBounce);
    lua["Ease"] = easeTable;

    lua.new_usertype<TweenInstance>("TweenInstance",
        sol::no_constructor,
        "SetEase", [](std::shared_ptr<TweenInstance> self, sol::object ease) {
            if (self) self->SetEase(ease);
            return self;
        },
        "SetDelay", [](std::shared_ptr<TweenInstance> self, float delay) {
            if (self) self->SetDelay(delay);
            return self;
        },
        "SetLoops", [](std::shared_ptr<TweenInstance> self, int loops, sol::optional<bool> yoyo) {
            if (self) self->SetLoops(loops, yoyo);
            return self;
        },
        "SetSpeed", [](std::shared_ptr<TweenInstance> self, float speed) {
            if (self) self->SetSpeed(speed);
            return self;
        },
        "OnUpdate", [](std::shared_ptr<TweenInstance> self, sol::protected_function cb) {
            if (self) self->OnUpdate(cb);
            return self;
        },
        "OnComplete", [](std::shared_ptr<TweenInstance> self, sol::protected_function cb) {
            if (self) self->OnComplete(cb);
            return self;
        },
        "OnLoop", [](std::shared_ptr<TweenInstance> self, sol::protected_function cb) {
            if (self) self->OnLoop(cb);
            return self;
        },
        "Pause", [](std::shared_ptr<TweenInstance> self) {
            if (self) self->Pause();
        },
        "Resume", [](std::shared_ptr<TweenInstance> self) {
            if (self) self->Resume();
        },
        "Cancel", [](std::shared_ptr<TweenInstance> self) {
            if (self) self->Cancel();
        },
        "Kill", [](std::shared_ptr<TweenInstance> self) {
            if (self) self->Kill();
        },
        "Complete", [](std::shared_ptr<TweenInstance> self) {
            if (self) self->Complete();
        },
        "IsPlaying", [](std::shared_ptr<TweenInstance> self) {
            return self ? self->IsPlaying() : false;
        },
        "IsPaused", [](std::shared_ptr<TweenInstance> self) {
            return self ? self->IsPaused() : false;
        },
        "GetProgress", [](std::shared_ptr<TweenInstance> self) {
            return self ? self->GetProgress() : 0.0f;
        },
        "GetId", [](std::shared_ptr<TweenInstance> self) {
            return self ? self->GetId() : 0ULL;
        }
    );

    sol::table tweenTable = lua.create_table();

    tweenTable["Value"] = [](sol::object from, sol::object to, float duration, sol::optional<sol::protected_function> onUpdate) {
        TweenValue tvFrom = TweenValue::FromLua(from);
        TweenValue tvTo = TweenValue::FromLua(to);
        sol::protected_function cb = onUpdate.has_value() ? onUpdate.value() : sol::protected_function();
        return TweenManager::Get().CreateValueTween(entt::null, tvFrom, tvTo, duration, cb);
    };

    tweenTable["Position"] = [](sol::object targetObj, const glm::vec3& toPos, float duration) -> std::shared_ptr<TweenInstance> {
        entt::entity target = ResolveEntityFromLua(targetObj);
        return TweenManager::Get().CreatePositionTween(target, toPos, duration);
    };

    tweenTable["Rotation"] = [](sol::object targetObj, sol::object toRot, float duration) -> std::shared_ptr<TweenInstance> {
        entt::entity target = ResolveEntityFromLua(targetObj);
        return TweenManager::Get().CreateRotationTween(target, toRot, duration);
    };

    tweenTable["Scale"] = [](sol::object targetObj, const glm::vec3& toScale, float duration) -> std::shared_ptr<TweenInstance> {
        entt::entity target = ResolveEntityFromLua(targetObj);
        return TweenManager::Get().CreateScaleTween(target, toScale, duration);
    };

    tweenTable["To"] = [](sol::table targetTable, sol::table propsTable, float duration) -> std::shared_ptr<TweenInstance> {
        entt::entity owner = entt::null;
        sol::object entObj = targetTable["entity"];
        if (entObj.valid())
        {
            owner = ResolveEntityFromLua(entObj);
        }
        return TweenManager::Get().CreateTableTween(owner, targetTable, propsTable, duration);
    };

    tweenTable["Cancel"] = [](sol::object obj) {
        if (obj.is<uint64_t>())
        {
            TweenManager::Get().Cancel(obj.as<uint64_t>());
        }
        else if (obj.is<std::shared_ptr<TweenInstance>>())
        {
            auto ptr = obj.as<std::shared_ptr<TweenInstance>>();
            if (ptr) ptr->Cancel();
        }
        else if (obj.is<TweenInstance*>())
        {
            auto* ptr = obj.as<TweenInstance*>();
            if (ptr) ptr->Cancel();
        }
    };
    tweenTable["Kill"] = tweenTable["Cancel"];

    tweenTable["CancelAll"] = []() {
        TweenManager::Get().CancelAll();
    };
    tweenTable["KillAll"] = tweenTable["CancelAll"];

    tweenTable["PauseAll"] = []() {
        TweenManager::Get().PauseAll();
    };

    tweenTable["ResumeAll"] = []() {
        TweenManager::Get().ResumeAll();
    };

    tweenTable["IsPlaying"] = [](sol::object obj) -> bool {
        if (obj.is<uint64_t>())
        {
            return TweenManager::Get().IsPlaying(obj.as<uint64_t>());
        }
        else if (obj.is<std::shared_ptr<TweenInstance>>())
        {
            auto ptr = obj.as<std::shared_ptr<TweenInstance>>();
            return ptr ? ptr->IsPlaying() : false;
        }
        else if (obj.is<TweenInstance*>())
        {
            auto* ptr = obj.as<TweenInstance*>();
            return ptr ? ptr->IsPlaying() : false;
        }
        return false;
    };

    lua["Tween"] = tweenTable;
}

}
