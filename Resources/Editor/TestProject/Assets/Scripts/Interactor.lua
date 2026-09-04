Interactor = {}
Interactor.transform = nil

function Interactor:OnCreate()
    self.transform = self.entity:GetComponent("TransformComponent")
end

function Interactor:OnUpdate(dt)
    if Input.IsKeyJustPressed(KeyCode.F) then
        if self.transform then
            local raycastResult = Physics.Raycast(self.transform.worldPosition, self.transform.forward, 5.0, false)
            
            if raycastResult and raycastResult.hit then
                raycastResult.entity:CallMethod("interact")
            end
        end
    end
end

return Interactor