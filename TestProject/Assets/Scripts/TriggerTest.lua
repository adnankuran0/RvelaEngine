TriggerTest = {}

function TriggerTest:OnCollisionEnter(collisionInfo)
    if collisionInfo.other:GetName() == "Character" then
        print("coin collected")
        self.scene:DestroyEntity(self.entity)
    end
end

return TriggerTest