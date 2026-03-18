local Test = {}

Test.toggle = false
Test.timer = 0
Test.comp = nil
Test.ent = nil

function Test:OnCreate()
   self.comp = self.entity:GetComponent("MaterialComponent")
   self.ent = self.scene:CreateEntity("Light")
   self.ent:AddComponent("PointLightComponent")
   self.ent:GetComponent("TransformComponent"):SetPosition(Vec3.new(0.0,2.0,0.0))
   
end

function Test:OnUpdate(dt) 
    self.timer = self.timer + dt

    if self.timer > 2.0 then
        self.scene:DestroyEntity(self.ent)
    end
end

return Test