Test = {}

function Test:OnCreate() 
    local parentEntity = self.scene:FindEntityByName("Character")
    local transform_comp = self.entity:GetComponent("TransformComponent")
    local pos = transform_comp.position
    self.entity:SetParent(parentEntity)
    transform_comp.position = Vec3.new(pos.x,pos.y + 4.0,pos.z)
end


return Test