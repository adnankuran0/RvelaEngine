local Test = {}

Test.toggle = false
Test.timer = 0
Test.comp = nil

function Test:OnCreate()
   self.comp = self.entity:GetComponent("MaterialComponent")
end

function Test:OnUpdate(dt) 
    self.timer = self.timer + dt

    local uvOffset = self.comp:GetUVOffset()
    uvOffset.x = uvOffset.x + 1 * dt
    self.comp:SetUVOffset(uvOffset)
    self.comp:SetNormalScale(self.timer)
end

return Test