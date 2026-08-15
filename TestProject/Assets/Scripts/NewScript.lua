CallbackTest = {}

function CallbackTest:OnCreate() 
    self.ae = self.entity:GetComponent("AudioEmitterComponent")
end

function CallbackTest:OnUpdate(dt)
    if Input.IsKeyJustPressed(KeyCode.E)then
        self.ae:Play()
    end
end

return CallbackTest