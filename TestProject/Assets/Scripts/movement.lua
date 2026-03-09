Player = {}

Player.isGrounded = true
Player.jumpStrength = 4.0

Player.acceleration = 9900.0
Player.deceleration = 9900.0
Player.maxWalkSpeed = 3.0
Player.maxSprintSpeed = 5.0

Player.inputDir = Vec3.new(0,0,0)
Player.wantJump = false

function Player:OnCreate()
    self.rb = self.entity:GetComponent("RigidbodyComponent")

    self.camHolder = self.scene:FindEntityByName("CameraHolder")
    self.cam = self.scene:FindEntityByName("Camera")

    self.headBobTime = 0.0

    if self.cam then
        local t = self.cam:GetComponent("TransformComponent")
        self.camStartY = t:GetPosition().y
    end
end

function Player:OnUpdate(dt)
    -- Input
    local camForward = Vec3.new(0,0,-1)
    local camRight   = Vec3.new(1,0,0)

    if self.camHolder then
        local camT = self.camHolder:GetComponent("TransformComponent")
        camForward = camT:GetForward()
        camForward.y = 0
        camForward = camForward:Normalized()
        camRight = Vec3.Cross(Vec3.UP(), camForward):Normalized()
    end

    local inputDir = Vec3.new(0,0,0)
    if Input.IsKeyPressed(KeyCode.W) then inputDir = inputDir + camForward end
    if Input.IsKeyPressed(KeyCode.S) then inputDir = inputDir - camForward end
    if Input.IsKeyPressed(KeyCode.A) then inputDir = inputDir - camRight end
    if Input.IsKeyPressed(KeyCode.D) then inputDir = inputDir + camRight end

    self.inputDir = inputDir

    if Input.IsKeyJustPressed(KeyCode.Space) then
        self.wantJump = true
    end

    if self.cam then
        local camComp = self.cam:GetComponent("CameraComponent")
        if camComp then
            local baseFOV = 75.0
            local sprintFOV = 90.0
            local fovSpeed = 10.0

            local targetFOV = Input.IsKeyPressed(KeyCode.LeftShift) and sprintFOV or baseFOV
            local currentFOV = camComp:GetFOV()
            local newFOV = currentFOV + (targetFOV - currentFOV) * math.min(1, fovSpeed * dt)
            camComp:SetFOV(newFOV)
        end

        local camT = self.cam:GetComponent("TransformComponent")
        local moving = inputDir:LengthSq() > 0
        local bobFreq = Input.IsKeyPressed(KeyCode.LeftShift) and 14.0 or 8.0
        local bobAmp  = Input.IsKeyPressed(KeyCode.LeftShift) and 0.01 or 0.03

        if moving then
            self.headBobTime = self.headBobTime + dt * bobFreq
        else
            self.headBobTime = self.headBobTime * 0.8
        end

        local offset = math.sin(self.headBobTime) * bobAmp
        local pos = camT:GetPosition()
        pos.y = self.camStartY + offset
        camT:SetPosition(pos)
    end
end

function Player:OnFixedUpdate(fixedDt)
    if not self.rb then return end

    local inputDir = self.inputDir
    if inputDir:LengthSq() > 0 then
        inputDir = inputDir:Normalized()
    end

    local targetSpeed = Input.IsKeyPressed(KeyCode.LeftShift) and self.maxSprintSpeed or self.maxWalkSpeed

    local velocity = self.physics:GetLinearVelocity(self.rb)
    local horizontal = Vec3.new(velocity.x, 0, velocity.z)

    if inputDir:LengthSq() > 0 then
        local targetVelocity = inputDir * targetSpeed
        local diff = targetVelocity - horizontal
        local force = diff * self.acceleration
        self.physics:AddForce(self.rb, Vec3.new(force.x, 0, force.z))
    else
        local damping = horizontal * -self.deceleration
        self.physics:AddForce(self.rb, Vec3.new(damping.x, 0, damping.z))
    end

    if self.wantJump and self.isGrounded then
        local v = self.physics:GetLinearVelocity(self.rb)
        v.y = self.jumpStrength
        self.physics:SetLinearVelocity(self.rb, v)
        self.isGrounded = false
    end
    self.wantJump = false -- reset jump buffer

    velocity = self.physics:GetLinearVelocity(self.rb)
    if math.abs(velocity.y) < 0.01 then
        self.isGrounded = true
    end
end

return Player