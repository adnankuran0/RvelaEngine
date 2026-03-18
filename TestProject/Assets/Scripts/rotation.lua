local Player = {}

local time = 0.0

function Player:OnCreate()
    print("Player created")
end

function Player:OnUpdate(dt)
    time = time + dt
    local transform = self.entity:GetComponent("TransformComponent")

    local rot = transform:GetEulerRotation()
    
    rot.x = time * 100
    rot.y = time * 100 + 180
    rot.z = time * 100 + 180
    
    transform:SetEulerRotation(rot)
end

return Player
