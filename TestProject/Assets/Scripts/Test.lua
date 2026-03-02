local Test = {}

Test.toggle = false
Test.timer = 0
Test.comp = nil

function Test:OnCreate()
   self.comp = self.entity:GetComponent("MaterialComponent")
end

function Test:OnUpdate(dt) 
end

return Test