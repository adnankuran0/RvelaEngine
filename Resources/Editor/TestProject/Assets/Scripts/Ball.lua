---@class Ball : ScriptInstance
local Ball = {}

Ball.properties = {
    speed = 10.0
}

function Ball:OnCreate()
    self.transform = self.entity:GetComponent("Transform")
end

function Ball:OnUpdate(dt)
	if not self.transform or not self.transform:IsValid() then return end
	self.transform:Translate(self.transform.forward * self.speed * dt)
end


return Ball
