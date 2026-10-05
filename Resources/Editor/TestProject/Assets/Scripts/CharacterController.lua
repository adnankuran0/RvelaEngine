---@class Player : ScriptInstance
---@field walkSpeed number
---@field sprintSpeed number
---@field jumpStrength number
---@field acceleration number
---@field deceleration number
---@field gravity number
---@field firePrefab AssetHandle
---@field transform TransformComponent
---@field cb CharacterBodyComponent
---@field ae AudioEmitterComponent
---@field camHolder Entity?
---@field cam Entity?
---@field headBobTime number
---@field camStartY number
Player = {}

Player.properties = {
    walkSpeed        = 3.0,
    sprintSpeed      = 5.0,
    jumpStrength     = 5.0,
    acceleration     = 20.0,
    deceleration     = 20.0,
    gravity          = -12.0,
    firePrefab       = { type = "AssetHandle"}
}

function Player:OnCreate()
    self.transform  = self.entity:GetComponent("Transform")
    self.cb         = self.entity:GetComponent("CharacterBody")
    self.ae         = self.entity:GetComponent("AudioEmitter")
    self.camHolder  = self.scene:FindEntityByName("CameraHolder")
    self.cam        = self.scene:FindEntityByName("Camera")
    self.headBobTime = 0.0

    if self.cam then
        local t = self.cam:GetComponent("Transform")
        self.camStartY = t.position.y
    end
end

function Player:OnUpdate(dt)
    if not self.cb then return end

    

    local camForward = Vec3.new(0, 0, -1)
    local camRight   = Vec3.new(1, 0,  0)

    if self.camHolder then
        local camT   = self.camHolder:GetComponent("Transform")
        camForward   = camT.forward
        camForward.y = 0
        camForward   = camForward:Normalized()
        camRight     = camT.right
    end

    if Input.IsMouseButtonJustPressed(MouseButton.Left) and self.transform then
        local aimForward = camForward
        local spawnRot = self.transform.worldRotation
        if self.cam then
            local camT = self.cam:GetComponent("Transform")
            if camT and camT:IsValid() then
                aimForward = camT.forward
                spawnRot = camT.worldRotation
            end
        elseif self.camHolder then
            local camT = self.camHolder:GetComponent("Transform")
            if camT and camT:IsValid() then
                spawnRot = camT.worldRotation
            end
        end
        local spawnPos = self.transform.worldPosition + aimForward
        if self.firePrefab and self.firePrefab:IsValid() then
            self.scene:Instantiate(self.firePrefab, spawnPos, spawnRot)
        end
    end

    local inputDir = Vec3.new(0, 0, 0)
    if Input.IsKeyPressed(KeyCode.W) then inputDir = inputDir + camForward end
    if Input.IsKeyPressed(KeyCode.S) then inputDir = inputDir - camForward end
    if Input.IsKeyPressed(KeyCode.A) then inputDir = inputDir - camRight   end
    if Input.IsKeyPressed(KeyCode.D) then inputDir = inputDir + camRight   end

    local isSprinting  = Input.IsKeyPressed(KeyCode.LeftShift)
    local targetSpeed  = isSprinting and self.sprintSpeed or self.walkSpeed
    
    local isGrounded   = self.cb.isGrounded

    local velocity   = self.cb.velocity
    local horizontal = Vec3.new(velocity.x, 0, velocity.z)

    local verticalVel = velocity.y
    if not isGrounded then
        verticalVel = verticalVel + self.gravity * dt
    else
        if verticalVel < 0 then verticalVel = 0 end
    end

    if inputDir:LengthSq() > 0 then
        inputDir = inputDir:Normalized()
        local targetVel = inputDir * targetSpeed
        local diff      = targetVel - horizontal
        ---@type Vec3
        local newH      = horizontal + diff * math.min(1.0, self.acceleration * dt)
        
        self.cb.velocity = Vec3.new(newH.x, verticalVel, newH.z)
    else
        ---@type Vec3
        local newH = horizontal * math.max(0.0, 1.0 - self.deceleration * dt)
        
        self.cb.velocity = Vec3.new(newH.x, verticalVel, newH.z)  
    end

    if Input.IsKeyJustPressed(KeyCode.Space) and isGrounded then
        self.ae:Play()
        local v = self.cb.velocity
        self.cb.velocity = Vec3.new(v.x, self.jumpStrength, v.z)
    end

    if self.cam then
        local camComp = self.cam:GetComponent("Camera")
        if camComp:IsValid() then
            local targetFOV  = isSprinting and 90.0 or 75.0
            local currentFOV = camComp.fov
            camComp.fov = currentFOV + (targetFOV - currentFOV) * math.min(1.0, 10.0 * dt)
        end
    end

    if self.cam then
        local camT   = self.cam:GetComponent("Transform")
        local moving = inputDir:LengthSq() > 0 and isGrounded
        local freq   = isSprinting and 14.0 or 8.0
        local amp    = isSprinting and 0.05 or 0.06

        if moving then
            local previousBob = math.sin(self.headBobTime)

            self.headBobTime = self.headBobTime + dt * freq

            local currentBob = math.sin(self.headBobTime)

            if previousBob > 0.0 and currentBob <= 0.0 then
                self.ae.pitch = 0.9 + math.random() * 0.20
                self.ae:Play()
            end
        else
            self.headBobTime = self.headBobTime * 0.85
        end

        local pos = camT.position
        pos.y = self.camStartY + math.sin(self.headBobTime) * amp
        camT.position = pos
    end
end

return Player
