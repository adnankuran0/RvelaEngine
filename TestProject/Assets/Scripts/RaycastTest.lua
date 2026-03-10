RaycastTest = {}
RaycastTest.transform = nil
RaycastTest.wantDestroy = false

function RaycastTest:OnCreate()
    self.transform = self.entity:GetComponent("TransformComponent")
end

function RaycastTest:OnUpdate(dt)
    if Input.IsKeyJustPressed(KeyCode.F) then
        self.wantDestroy = true
    end

end

function RaycastTest:OnFixedUpdate(dt)
    if self.wantDestroy then
        local raycastResult = self.physics:Raycast(self.transform:GetWorldPosition(),self.transform:GetForward(),5.0,false)
        if raycastResult.hit and raycastResult.entity:GetName() ~= "Terrain" then
            print("Found hit",raycastResult.entity:GetName(),raycastResult.point)
            self.scene:DestroyEntity(raycastResult.entity)
        end

        self.wantDestroy = false
    end
end

return RaycastTest