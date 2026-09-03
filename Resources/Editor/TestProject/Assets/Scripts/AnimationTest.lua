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

return AnimationTest