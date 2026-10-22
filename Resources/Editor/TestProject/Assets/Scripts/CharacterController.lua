---@class Player : ScriptInstance
---@field walkSpeed number
---@field sprintSpeed number
---@field crouchSpeed number
---@field crouchTransitionSpeed number
---@field standHalfHeight number
---@field crouchHalfHeight number
---@field crouchCamOffset number
---@field jumpStrength number
---@field groundAccel number
---@field airAccel number
---@field maxAirWishSpeed number
---@field friction number
---@field stopSpeed number
---@field acceleration number
---@field deceleration number
---@field gravity number
---@field firePrefab AssetHandle
---@field transform TransformComponent
---@field cb CharacterBodyComponent
---@field capsule CapsuleColliderComponent
---@field ae AudioEmitterComponent
---@field camHolder Entity?
---@field cam Entity?
---@field headBobTime number
---@field camStartY number
---@field camHolderStartY number
---@field crouchProgress number
---@field appliedHalfHeight number
---@field standOffset Vec3
---@field radius number
Player = {}

Player.className = "CharacterController"

Player.properties = {
    walkSpeed             = 3.5,
    sprintSpeed           = 6.0,
    crouchSpeed           = 1.8,
    crouchTransitionSpeed = 8.0,
    standHalfHeight       = 0.5,
    crouchHalfHeight      = 0.25,
    crouchCamOffset       = 0.45,
    jumpStrength          = 5.2,
    groundAccel           = 12.0,
    airAccel              = 16.0,
    maxAirWishSpeed       = 3.0,
    friction              = 6.0,
    stopSpeed             = 1.5,
    gravity               = -15.0,
    acceleration          = 12.0,
    deceleration          = 6.0,
    firePrefab            = { type = "AssetHandle"},
}

---Apply ground friction (Source PM_Friction)
---@param vel Vec3
---@param dt number
---@param friction number
---@param stopSpeed number
---@return Vec3
local function ApplyFriction(vel, dt, friction, stopSpeed)
    local speed = vel:Length()
    if speed < 0.0001 then
        return Vec3.new(0, 0, 0)
    end

    local control = speed < stopSpeed and stopSpeed or speed
    local drop = control * friction * dt
    local newSpeed = math.max(0.0, speed - drop)
    return vel * (newSpeed / speed)
end

---Source PM_Accelerate / PM_AirAccelerate
---@param currentVel Vec3 Current horizontal velocity
---@param wishDir Vec3 Normalized wish direction
---@param wishSpeed number Target maximum speed in wish direction
---@param accel number Acceleration rate
---@param dt number Delta time
---@return Vec3
local function Accelerate(currentVel, wishDir, wishSpeed, accel, dt)
    -- Project current velocity onto wish direction
    local currentSpeed = currentVel:Dot(wishDir)
    local addSpeed = wishSpeed - currentSpeed

    if addSpeed <= 0 then
        return currentVel
    end

    local accelSpeed = accel * wishSpeed * dt
    if accelSpeed > addSpeed then
        accelSpeed = addSpeed
    end

    return currentVel + wishDir * accelSpeed
end

function Player:OnCreate()
    self.transform       = self.entity:GetComponent("Transform")
    self.cb              = self.entity:GetComponent("CharacterBody")
    self.capsule         = self.entity:GetComponent("CapsuleCollider")
    self.ae              = self.entity:GetComponent("AudioEmitter")
    self.camHolder       = self.scene:FindEntityByName("CameraHolder")
    self.cam             = self.scene:FindEntityByName("Camera")
    self.headBobTime     = 0.0
    self.crouchProgress  = 0.0

    local radius = 0.35
    if self.capsule and self.capsule:IsValid() then
        self.standHalfHeight = self.capsule.halfHeight
        self.standOffset     = self.capsule.offset
        radius               = self.capsule.radius
    else
        self.standOffset     = Vec3.new(0, 0, 0)
    end
    self.radius = radius
    self.appliedHalfHeight = self.standHalfHeight

    if self.cam then
        local t = self.cam:GetComponent("Transform")
        if t and t:IsValid() then
            self.camStartY = t.position.y
        else
            self.camStartY = 0.0
        end
    end

    self.camHolderStartY = 0.51
    if self.camHolder then
        local t = self.camHolder:GetComponent("Transform")
        if t and t:IsValid() then
            self.camHolderStartY = t.position.y
        end
    end
end

---Check if there is enough overhead clearance to stand up without penetrating ceiling/walls
---@return boolean true if clear to stand up, false if blocked by an obstacle overhead
function Player:CheckCeilingClearance()
    local radius = self.radius or 0.35
    local standH = self.standHalfHeight or 0.5
    local totalStandHeight = (standH + radius) * 2.0 -- ~1.70m

    local worldPos = self.transform.worldPosition
    -- Bottom of the capsule (feet on ground)
    local feetY = worldPos.y - (standH + radius)
    -- Start ray well below crouched head (crouch height is ~1.20m), 35cm above feet
    local startY = feetY + 0.35
    -- Top of the standing capsule + 10cm safety margin
    local targetTopY = feetY + totalStandHeight + 0.10
    local rayDist = targetTopY - startY -- ~1.45m

    local camForward = Vec3.new(0, 0, -1)
    local camRight   = Vec3.new(1, 0,  0)
    if self.camHolder then
        local camT = self.camHolder:GetComponent("Transform")
        if camT and camT:IsValid() then
            camForward = camT.forward
            camForward.y = 0
            camForward = camForward:Normalized()
            camRight = camT.right
            camRight.y = 0
            camRight = camRight:Normalized()
        end
    end

    -- Multi-ray pattern covering the full capsule diameter (center + 4 cardinal directions)
    local r = radius * 0.75
    local offsets = {
        Vec3.new(0, 0, 0),
        camForward * r,
        camForward * (-r),
        camRight * r,
        camRight * (-r),
    }

    local upDir = Vec3.new(0, 1, 0)

    for i = 1, #offsets do
        local off = offsets[i]
        local origin = Vec3.new(worldPos.x + off.x, startY, worldPos.z + off.z)
        local hit = Physics.Raycast(origin, upDir, rayDist, true)
        if hit and hit.hit then
            -- Obstacle detected above head within standing height!
            return false
        end
    end

    return true
end

function Player:OnUpdate(dt)
    if not self.cb then return end

    local camForward = Vec3.new(0, 0, -1)
    local camRight   = Vec3.new(1, 0,  0)

    if self.camHolder then
        local camT   = self.camHolder:GetComponent("Transform")
        if camT and camT:IsValid() then
            camForward   = camT.forward
            camForward.y = 0
            camForward   = camForward:Normalized()
            camRight     = camT.right
        end
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

    -- Crouch Input & Overhead Clearance Check
    local wantsToCrouch = Input.IsKeyPressed(KeyCode.LeftControl) or Input.IsKeyPressed(KeyCode.RightControl)
    local canUncrouch = true

    -- If currently crouched or in the process of uncrouching, verify overhead clearance
    if self.crouchProgress > 0.001 then
        canUncrouch = self:CheckCeilingClearance()
    end

    -- If user wants to crouch OR ceiling blocks standing up, target crouch remains 1.0
    local targetCrouch = (wantsToCrouch or not canUncrouch) and 1.0 or 0.0

    local crouchSpeed = self.crouchTransitionSpeed or 8.0
    if self.crouchProgress < targetCrouch then
        self.crouchProgress = math.min(targetCrouch, self.crouchProgress + crouchSpeed * dt)
    elseif self.crouchProgress > targetCrouch then
        self.crouchProgress = math.max(targetCrouch, self.crouchProgress - crouchSpeed * dt)
    end

    local p = self.crouchProgress
    local smoothT = p * p * (3.0 - 2.0 * p) -- Smoothstep

    -- Update Collider Dimensions
    local currentHalfHeight = self.standHalfHeight + (self.crouchHalfHeight - self.standHalfHeight) * smoothT
    local heightDiff = self.standHalfHeight - currentHalfHeight
    local currentOffsetY = (self.standOffset and self.standOffset.y or 0.0) - heightDiff

    if self.capsule and self.capsule:IsValid() then
        if math.abs(self.appliedHalfHeight - currentHalfHeight) > 0.0005 then
            self.capsule.halfHeight = currentHalfHeight
            self.capsule.offset = Vec3.new(
                self.standOffset and self.standOffset.x or 0.0,
                currentOffsetY,
                self.standOffset and self.standOffset.z or 0.0
            )
            self.appliedHalfHeight = currentHalfHeight
        end
    end

    -- Update Camera Holder Height
    if self.camHolder then
        local camHT = self.camHolder:GetComponent("Transform")
        if camHT and camHT:IsValid() then
            local pos = camHT.position
            pos.y = self.camHolderStartY - (self.crouchCamOffset * smoothT)
            camHT.position = pos
        end
    end

    -- Input Direction
    local inputDir = Vec3.new(0, 0, 0)
    if Input.IsKeyPressed(KeyCode.W) then inputDir = inputDir + camForward end
    if Input.IsKeyPressed(KeyCode.S) then inputDir = inputDir - camForward end
    if Input.IsKeyPressed(KeyCode.A) then inputDir = inputDir - camRight   end
    if Input.IsKeyPressed(KeyCode.D) then inputDir = inputDir + camRight   end

    local isGrounded   = self.cb.isGrounded
    local isCrouching  = self.crouchProgress > 0.5
    local isSprinting  = Input.IsKeyPressed(KeyCode.LeftShift) and not isCrouching

    local baseSpeed    = isSprinting and self.sprintSpeed or self.walkSpeed
    local targetSpeed  = baseSpeed + (self.crouchSpeed - self.walkSpeed) * smoothT

    local velocity     = self.cb.velocity
    local horizontal   = Vec3.new(velocity.x, 0, velocity.z)
    local verticalVel  = velocity.y

    -- Gravity
    if not isGrounded then
        verticalVel = verticalVel + self.gravity * dt
    else
        if verticalVel < 0 then verticalVel = 0 end
    end

    -- Jump (Source style: Preserve 100% of momentum, zero takeoff friction)
    local jumpedThisFrame = false
    if Input.IsKeyJustPressed(KeyCode.Space) and isGrounded then
        self.ae:Play()
        verticalVel = self.jumpStrength
        isGrounded = false
        jumpedThisFrame = true
    end

    -- Source Engine Physics Movement
    local groundAccelVal = self.groundAccel or self.acceleration or 12.0
    local airAccelVal    = self.airAccel or 16.0
    local frictionVal    = self.friction or (self.deceleration and (self.deceleration / 3.0)) or 6.0
    local stopSpeedVal   = self.stopSpeed or 1.5
    local maxAirSpeed    = self.maxAirWishSpeed or 3.0

    if isGrounded then
        -- 1. Apply ground friction (skip on takeoff frame so full horizontal speed is launched)
        if not jumpedThisFrame then
            horizontal = ApplyFriction(horizontal, dt, frictionVal, stopSpeedVal)
        end

        -- 2. Ground Acceleration (PM_Accelerate)
        if inputDir:LengthSq() > 0 then
            inputDir = inputDir:Normalized()
            horizontal = Accelerate(horizontal, inputDir, targetSpeed, groundAccelVal, dt)
        end
    else
        -- Air Movement (Source PM_AirAccelerate)
        -- ZERO ground friction in air! Horizontal momentum is 100% conserved!
        if inputDir:LengthSq() > 0 then
            inputDir = inputDir:Normalized()
            -- In Source air, wishSpeed is capped to maxAirWishSpeed for strafing
            local airWishSpeed = math.min(targetSpeed, maxAirSpeed)
            horizontal = Accelerate(horizontal, inputDir, airWishSpeed, airAccelVal, dt)
        end
    end

    self.cb.velocity = Vec3.new(horizontal.x, verticalVel, horizontal.z)

    -- Dynamic FOV (Source / FPS feel)
    if self.cam then
        local camComp = self.cam:GetComponent("Camera")
        if camComp:IsValid() then
            local targetFOV = (isSprinting or (not isGrounded and horizontal:Length() > self.walkSpeed + 0.5)) and 90.0 or (isCrouching and 72.0 or 75.0)
            local currentFOV = camComp.fov
            camComp.fov = currentFOV + (targetFOV - currentFOV) * math.min(1.0, 10.0 * dt)
        end
    end

    -- Head Bobbing (only on ground)
    if self.cam then
        local camT   = self.cam:GetComponent("Transform")
        local moving = inputDir:LengthSq() > 0 and isGrounded
        local freq   = isSprinting and 14.0 or (isCrouching and 6.0 or 8.0)
        local amp    = isSprinting and 0.05 or (isCrouching and 0.025 or 0.06)

        if moving then
            local previousBob = math.sin(self.headBobTime)

            self.headBobTime = self.headBobTime + dt * freq

            local currentBob = math.sin(self.headBobTime)

            if previousBob > 0.0 and currentBob <= 0.0 then
                self.ae.pitch = 0.9 + math.random() * 0.20
                if isCrouching then
                    self.ae.volume = 0.8
                else
                    self.ae.volume = 2.0
                end
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
