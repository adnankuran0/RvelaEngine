CallbackTest = {}

CallbackTest.destroyTimer = nil
CallbackTest.targetEntity = nil


function CallbackTest:OnCollisionEnter(collisionInfo)
    self.destroyTimer = 2.0
    self.targetEntity = collisionInfo.other
    if self.targetEntity:GetName() == "Cube" then
        self.scene:DestroyEntity(self.targetEntity)

    end
end
return CallbackTest
