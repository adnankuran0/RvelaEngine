RaycastTest = {}
RaycastTest.transform = nil

function RaycastTest:OnCreate()
    self.transform = self.entity:GetComponent("TransformComponent")
end

function RaycastTest:OnFixedUpdate(dt)
    local raycastResult = self.physics:Raycast(self.transform:GetWorldPosition(),self.transform:GetForward(),5.0,false)
    if raycastResult.hit then
        print("Found hit",raycastResult.entity:GetName(),raycastResult.point)
        
    end

end

return RaycastTest