AnimationTest = {}

function AnimationTest:OnCreate()
    self.animator = self.entity:GetComponent("AnimatorComponent")
end

function AnimationTest:OnUpdate(dt)
    if Input.IsKeyJustPressed(KeyCode.G) then
        self.animator:Play("Grow")
    end
end


function AnimationTest:OnAnimationEvent(eventName, parameter)
    if eventName == "PlaySound" then
        print("ta daa")
    end
end

function AnimationTest:OnAnimationStarted(clipName)
    print("Anim started: " .. clipName)
end

function AnimationTest:OnAnimationFinished(clipName)
    print("Anim finished: " .. clipName)
    if clipName == "Grow" then
        self.scene:DestroyEntity(self.entity)
    end
end

function AnimationTest:OnAnimationLooped(clipName)
    print("Anim looped: " .. clipName)
end

return AnimationTest