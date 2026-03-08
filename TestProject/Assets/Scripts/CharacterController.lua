Player = {}

Player.jumpStrength     = 6.0
Player.walkSpeed        = 3.0
Player.sprintSpeed      = 5.0
Player.acceleration     = 20.0
Player.deceleration     = 20.0
Player.gravity = -20.0

function Player:OnCreate()
    self.cb = self.entity:GetComponent("CharacterBodyComponent")
    self.camHolder  = self.scene:FindEntityByName("CameraHolder")
    self.cam        = self.scene:FindEntityByName("Camera")
    self.headBobTime = 0.0

    if self.cam then
        local t = self.cam:GetComponent("TransformComponent")
        self.camStartY = t:GetPosition().y
    end
end

function Player:OnUpdate(dt)
    if not self.cb then return end

    local camForward = Vec3.new(0, 0, -1)
    local camRight   = Vec3.new(1, 0,  0)

    if self.camHolder then
        local camT   = self.camHolder:GetComponent("TransformComponent")
        camForward   = camT:GetForward()
        camForward.y = 0
        camForward   = camForward:Normalized()
        camRight     = Vec3.Cross(Vec3.UP(), camForward):Normalized()
    end

    local inputDir = Vec3.new(0, 0, 0)
    if Input.IsKeyPressed(KeyCode.W) then inputDir = inputDir + camForward end
    if Input.IsKeyPressed(KeyCode.S) then inputDir = inputDir - camForward end
    if Input.IsKeyPressed(KeyCode.A) then inputDir = inputDir + camRight   end
    if Input.IsKeyPressed(KeyCode.D) then inputDir = inputDir - camRight   end

    local isSprinting  = Input.IsKeyPressed(KeyCode.LeftShift)
    local targetSpeed  = isSprinting and self.sprintSpeed or self.walkSpeed
    local isGrounded   = self.physics:IsCharacterGrounded(self.cb)

    local velocity   = self.physics:GetCharacterLinearVelocity(self.cb)
    local horizontal = Vec3.new(velocity.x, 0, velocity.z)

    local verticalVel = velocity.y
    if not isGrounded then
        verticalVel = verticalVel + Player.gravity * dt
    else
        if verticalVel < 0 then verticalVel = 0 end
    end

    if inputDir:LengthSq() > 0 then
        inputDir = inputDir:Normalized()
        local targetVel = inputDir * targetSpeed
        local diff      = targetVel - horizontal
        local newH      = horizontal + diff * math.min(1.0, self.acceleration * dt)
        self.physics:SetCharacterLinearVelocity(self.cb,
            Vec3.new(newH.x, verticalVel, newH.z)) 
    else
        local newH = horizontal * math.max(0.0, 1.0 - self.deceleration * dt)
        self.physics:SetCharacterLinearVelocity(self.cb,
            Vec3.new(newH.x, verticalVel, newH.z))  
    end

    if Input.IsKeyJustPressed(KeyCode.Space) and isGrounded then
        local v = self.physics:GetCharacterLinearVelocity(self.cb)
        self.physics:SetCharacterLinearVelocity(self.cb,
            Vec3.new(v.x, self.jumpStrength, v.z))
    end

    if self.cam then
        local camComp = self.cam:GetComponent("CameraComponent")
        if camComp then
            local targetFOV  = isSprinting and 90.0 or 75.0
            local currentFOV = camComp:GetFOV()
            camComp:SetFOV(currentFOV + (targetFOV - currentFOV) * math.min(1.0, 10.0 * dt))
        end
    end

    if self.cam then
        local camT   = self.cam:GetComponent("TransformComponent")
        local moving = inputDir:LengthSq() > 0 and isGrounded
        local freq   = isSprinting and 14.0 or 8.0
        local amp    = isSprinting and 0.01 or 0.03

        if moving then
            self.headBobTime = self.headBobTime + dt * freq
        else
            self.headBobTime = self.headBobTime * 0.85
        end

        local pos = camT:GetPosition()
        pos.y = self.camStartY + math.sin(self.headBobTime) * amp
        camT:SetPosition(pos)
    end
end

return Player