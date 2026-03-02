Player = {}

Player.velocity = Vec3.new(0,0,0)
Player.velocityY = 0

Player.isGrounded = true
Player.jumpStrength = 5.0
Player.gravity = -9.8

Player.acceleration = 20.0
Player.deceleration = 25.0
Player.maxWalkSpeed = 3.0
Player.maxSprintSpeed = 5.0

function Player:OnCreate()
    self.cam = self.scene:FindEntityByName("CameraHolder")
    if not self.cam then
        print("Warning: CameraHolder not found on start")
    end
end

function Player:OnUpdate(dt)
    local transform = self.entity:GetComponent("Transform")

    local targetSpeed = self.maxWalkSpeed
    if Input.IsKeyPressed(KeyCode.LeftShift) then
        targetSpeed = self.maxSprintSpeed
    end

    local camForward = Vec3.new(0,0,-1)
    local camRight   = Vec3.new(1,0,0)

    if self.cam then
        local camTransform = self.cam:GetComponent("Transform")

        camForward = camTransform:GetForward()
        camForward.y = 0
        camForward = camForward:Normalized()

        camRight = Vec3.Cross(Vec3.UP(), camForward):Normalized()
    end

    local inputDir = Vec3.new(0,0,0)

    if Input.IsKeyPressed(KeyCode.W) then
        inputDir = inputDir + camForward
    end
    if Input.IsKeyPressed(KeyCode.S) then
        inputDir = inputDir - camForward
    end
    if Input.IsKeyPressed(KeyCode.A) then
        inputDir = inputDir + camRight
    end
    if Input.IsKeyPressed(KeyCode.D) then
        inputDir = inputDir - camRight
    end

    if inputDir:LengthSq() > 0 then
        inputDir = inputDir:Normalized()
        local targetVelocity = inputDir * targetSpeed

        local diff = targetVelocity - Vec3.new(self.velocity.x,0,self.velocity.z)
        local accel = diff * math.min(1, self.acceleration * dt)

        self.velocity.x = self.velocity.x + accel.x
        self.velocity.z = self.velocity.z + accel.z
    else
        local horizontal = Vec3.new(self.velocity.x,0,self.velocity.z)
        local decay = math.min(1, self.deceleration * dt)

        horizontal = horizontal * (1 - decay)

        self.velocity.x = horizontal.x
        self.velocity.z = horizontal.z
    end

    if Input.IsKeyJustPressed(KeyCode.Space) and self.isGrounded then
        self.velocityY = self.jumpStrength
        self.isGrounded = false
    end

    self.velocityY = self.velocityY + self.gravity * dt
    self.velocity.y = self.velocityY

    transform:Translate(self.velocity * dt)

    if transform:GetPosition().y <= 1 then
        local pos = transform:GetPosition()
        pos.y = 1
        transform:SetPosition(pos)

        self.velocityY = 0
        self.velocity.y = 0
        self.isGrounded = true
    end
end

return Player