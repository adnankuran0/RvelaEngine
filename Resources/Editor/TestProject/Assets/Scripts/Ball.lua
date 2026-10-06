---@class Ball : ScriptInstance
local Ball = {}

Ball.properties = {
    speed = 100.0,
    destroySecons = 2
}

function Ball:OnCreate()
    self:StartTimer(self.destroySecons,function (self)
        self.entity:Destroy()
    end)
end

function Ball:OnReady()
    self.transform = self.entity:GetComponent("Transform")
    self.rb = self.entity:GetComponent("Rigidbody")
    self.rb:AddImpulse(self.transform.forward * self.speed)
end


return Ball
