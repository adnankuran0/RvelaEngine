Camera = {}

Camera.yaw = -90.0
Camera.pitch = 0.0
Camera.sensitivity = 0.1

Camera.lastMouseX = 0
Camera.lastMouseY = 0
Camera.firstMouse = true

Camera.captureMouse = true

function Camera:OnCreate()
    Input.SetMouseMode(MouseMode.CAPTURED)
end

function Camera:OnUpdate(dt)
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
    if self.pitch < -45.0 then
        self.pitch = -45.0
    end

    transform:SetEulerRotation(Vec3.new(self.pitch, self.yaw, 0.0))
end

return Camera