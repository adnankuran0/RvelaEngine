AnimationTest = {}

function AnimationTest:OnCreate()
    self.animator = self.entity:GetComponent("AnimatorComponent")
end

function AnimationTest:OnUpdate(dt)
    print(self.animator.isPlaying)

    if Input.IsKeyJustPressed(KeyCode.G) then
        self.animator:Play("Grow")
    end

end



return AnimationTest