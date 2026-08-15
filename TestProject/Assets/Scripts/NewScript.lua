CallbackTest = {}


function CallbackTest:OnCreate()
    self.ae = self.entity:GetComponent("AudioEmitterComponent")
end

function CallbackTest:OnCollisionEnter(collisionInfo)
    self.targetEntity = collisionInfo.other
    self.ae:Play()
    print(self.targetEntity:GetName())
end
return CallbackTest