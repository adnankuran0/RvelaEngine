local Player = {}

local time = 0.0

function Player:OnCreate()
    print("Player created")
end

function Player:OnUpdate(dt)
    time = time + dt
    local transform = self.entity:GetComponent("TransformComponent")

    local pos = transform:GetPosition()
    
    pos.y = math.sin(time * 2.0) * 2.0  

    transform:SetPosition(pos)
end

return Player
