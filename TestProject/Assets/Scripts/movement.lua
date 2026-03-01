Player = {}

Player.velocityY = 0
Player.isGrounded = true
Player.jumpStrength = 5.0
Player.gravity = -9.8

function Player:OnCreate()
end

function Player:OnUpdate(dt)
    local transform = self.entity:GetComponent("Transform")
    local walk_speed = 3.0
    local sprint_speed = 5.0
    local speed = walk_speed

    if Input.IsKeyPressed(KeyCode.LeftShift) then
        speed = sprint_speed
    end

    local cam = Scene.FindEntityByName("CameraHolder")
    local camForward = vec3.new(0,0,-1) 
    local camRight = vec3.new(1,0,0)

    if cam then
        local camTransform = cam:GetComponent("Transform")
        camForward = camTransform:GetForward()
        camForward.y = 0 
        camForward = camForward:Normalized()

        camRight = vec3.Cross(vec3.new(0,1,0), camForward):Normalized()
    else
        print("Can not find camera entity")
    end

    local moveDir = vec3.new(0,0,0)
    if Input.IsKeyPressed(KeyCode.W) then
        moveDir = moveDir + camForward
    end
    if Input.IsKeyPressed(KeyCode.S) then
        moveDir = moveDir - camForward
    end
    if Input.IsKeyPressed(KeyCode.A) then
        moveDir = moveDir + camRight
    end
    if Input.IsKeyPressed(KeyCode.D) then
        moveDir = moveDir - camRight
    end

    if moveDir.x ~= 0 or moveDir.z ~= 0 then
        moveDir = moveDir:Normalized() * speed * dt
        transform:Translate(vec3.new(moveDir.x, 0, moveDir.z))
    end

    if Input.IsKeyJustPressed(KeyCode.Space) and self.isGrounded then
        self.velocityY = self.jumpStrength
        self.isGrounded = false
    end

    self.velocityY = self.velocityY + self.gravity * dt
    transform:Translate(vec3.new(0, self.velocityY * dt, 0))

    if transform:GetPosition().y <= 1 then
        local pos = transform:GetPosition()
        pos.y = 1
        transform:SetPosition(pos)
        self.velocityY = 0
        self.isGrounded = true
    end
end

return Player