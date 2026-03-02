local Test = {}

function Test:OnCreate()
    local name = self.entity:GetName()
    print("Entity name: ", name)
    if  self.entity:HasComponent("TransformComponent") then
        local transform = self.entity:GetComponent("TransformComponent")
        local pos = transform:GetPosition()
        pos.x = pos.x + 5
        transform:SetPosition(pos)
    end
end

return Test