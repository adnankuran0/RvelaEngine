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
        local raycastResult = self.physics:Raycast(self.transform.worldPosition, self.transform.forward, 5.0, false)
        if raycastResult.hit and raycastResult.entity.name ~= "Terrain" then
            print("Found hit", raycastResult.entity.name, raycastResult.point)
            self.scene:DestroyEntity(raycastResult.entity)
        end

        self.wantDestroy = false
    end
end

return RaycastTest