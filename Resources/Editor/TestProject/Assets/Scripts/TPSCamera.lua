TPSCamera = {}

TPSCamera.yaw = -90.0
TPSCamera.pitch = 0.0
TPSCamera.sensitivity = 0.1

TPSCamera.lastMouseX = 0
TPSCamera.lastMouseY = 0
TPSCamera.firstMouse = true

TPSCamera.captureMouse = true

TPSCamera.desiredDistance = 5.0 
TPSCamera.minDistance = 0.5
TPSCamera.collisionBuffer = 0.2
TPSCamera.collisionSmoothSpeed = 30.0

TPSCamera.currentDistance = TPSCamera.desiredDistance
TPSCamera.zSign = 1.0

function TPSCamera:OnCreate()
    Input.SetMouseMode(MouseMode.CAPTURED)
    self.TPSCamera = self.scene:FindEntityByName("Camera")

    if self.TPSCamera then
        self.camTransform = self.TPSCamera:GetComponent("TransformComponent")

        if self.camTransform then
            local localPos = self.camTransform.position
            self.localX = localPos.x
            self.localY = localPos.y

            local z = localPos.z
            if math.abs(z) > 0.01 then
                self.desiredDistance = math.abs(z)
                self.zSign = z / math.abs(z)
            end
        end
    end

    self.currentDistance = self.desiredDistance
end

function TPSCamera:OnUpdate(dt)
    if Input.IsKeyJustPressed(KeyCode.Escape) then
        self.captureMouse = not self.captureMouse

        if self.captureMouse then
            Input.SetMouseMode(MouseMode.CAPTURED)
        else
            Input.SetMouseMode(MouseMode.VISIBLE)
        end
    end

    if not self.captureMouse then
        return
    end

    local transform = self.entity:GetComponent("TransformComponent")

    local mouse = Input.GetMousePosition()
    local mouseX = mouse.x
    local mouseY = mouse.y

    if self.firstMouse then
        self.lastMouseX = mouseX
        self.lastMouseY = mouseY
        self.firstMouse = false
    end

    local offsetX = mouseX - self.lastMouseX
    local offsetY = self.lastMouseY - mouseY

    self.lastMouseX = mouseX
    self.lastMouseY = mouseY

    offsetX = offsetX * self.sensitivity
    offsetY = offsetY * self.sensitivity

    self.yaw = self.yaw - offsetX
    self.pitch = self.pitch + offsetY

    if self.pitch > 89.0 then
        self.pitch = 89.0
    end
    if self.pitch < -89.0 then
        self.pitch = -89.0
    end

    transform.eulerRotation = Vec3.new(self.pitch, self.yaw, 0.0)

    if self.camTransform then
        local origin = transform.worldPosition
        local backDir = -transform.forward

        if backDir:LengthSq() > 0 then
            backDir = backDir:Normalized()
        end

        local hit = Physics.Raycast(origin, backDir, self.desiredDistance, false)

        local targetDistance = self.desiredDistance
        if hit.hit then
            targetDistance = math.max(self.minDistance, hit.distance - self.collisionBuffer)
        end

        local t = math.min(1.0, self.collisionSmoothSpeed * dt)
        self.currentDistance = self.currentDistance + (targetDistance - self.currentDistance) * t

        self.camTransform.position = Vec3.new(self.localX, self.localY, self.zSign * self.currentDistance)
    end
end

return TPSCamera