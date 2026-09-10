TPSCharacter = {}

TPSCharacter.jumpStrength = 5.0
TPSCharacter.walkSpeed = 1.0
TPSCharacter.sprintSpeed = 6.0
TPSCharacter.acceleration = 20.0
TPSCharacter.deceleration = 20.0
TPSCharacter.gravity = -12.0
TPSCharacter.rotationSpeed = 12.0

function TPSCharacter:OnCreate()
    self.cb = self.entity:GetComponent("CharacterBodyComponent")

    self.camHolder = self.scene:FindEntityByName("CameraHolder")
    self.mesh = self.scene:FindEntityByName("Mesh")

    if self.mesh then
        self.animator = self.mesh:GetComponent("AnimatorComponent")
    end

    self.wasGrounded = true
    self.isJumping = false
    self.currentAnimation = nil
end

function TPSCharacter:PlayAnimation(name)
    if not self.animator then
        return
    end

    if self.currentAnimation == name then
        return
    end

    self.currentAnimation = name
    self.animator:Play(name,0.15)
end

function TPSCharacter:OnUpdate(dt)
    if not self.cb then
        return
    end

    local camForward = Vec3.new(0, 0, -1)
    local camRight = Vec3.new(1, 0, 0)

    if self.camHolder then
        local camT = self.camHolder:GetComponent("TransformComponent")

        if camT then
            camForward = camT.forward
            camForward.y = 0

            if camForward:LengthSq() > 0 then
                camForward = camForward:Normalized()
            end

            camRight = camT.right
            camRight.y = 0

            if camRight:LengthSq() > 0 then
                camRight = camRight:Normalized()
            end
        end
    end

    local inputDir = Vec3.new(0, 0, 0)

    if Input.IsKeyPressed(KeyCode.W) then
        inputDir = inputDir + camForward
    end

    if Input.IsKeyPressed(KeyCode.S) then
        inputDir = inputDir - camForward
    end

    if Input.IsKeyPressed(KeyCode.A) then
        inputDir = inputDir - camRight
    end

    if Input.IsKeyPressed(KeyCode.D) then
        inputDir = inputDir + camRight
    end

    local isSprinting = Input.IsKeyPressed(KeyCode.LeftShift)
    local targetSpeed = isSprinting and self.sprintSpeed or self.walkSpeed

    local isGrounded = self.cb.isGrounded
    local velocity = self.cb.velocity

    local horizontal = Vec3.new(velocity.x, 0, velocity.z)
    local verticalVel = velocity.y

    if not isGrounded then
        verticalVel = verticalVel + self.gravity * dt
    elseif verticalVel < 0 then
        verticalVel = 0
    end

    local hasInput = inputDir:LengthSq() > 0

    if hasInput then
        inputDir = inputDir:Normalized()

        local targetVel = inputDir * targetSpeed
        local diff = targetVel - horizontal

        local newH = horizontal + diff * math.min(
            1.0,
            self.acceleration * dt
        )

        self.cb.velocity = Vec3.new(
            newH.x,
            verticalVel,
            newH.z
        )

        if self.mesh then
            local meshT = self.mesh:GetComponent("TransformComponent")

            if meshT then
                local targetYaw = math.deg(math.atan(
                    inputDir.x,
                    inputDir.z
                ))

                local currentYaw = meshT.eulerRotation.y
                local angleDiff = targetYaw - currentYaw

                if angleDiff > 180 then
                    angleDiff = angleDiff - 360
                elseif angleDiff < -180 then
                    angleDiff = angleDiff + 360
                end

                local newYaw = currentYaw + angleDiff * math.min(
                    1.0,
                    self.rotationSpeed * dt
                )

                meshT.eulerRotation = Vec3.new(0.0, newYaw, 0.0)
            end
        end
    else
        local newH = horizontal * math.max(
            0.0,
            1.0 - self.deceleration * dt
        )

        self.cb.velocity = Vec3.new(
            newH.x,
            verticalVel,
            newH.z
        )
    end

    if Input.IsKeyJustPressed(KeyCode.Space) and isGrounded then
        local v = self.cb.velocity

        self.cb.velocity = Vec3.new(
            v.x,
            self.jumpStrength,
            v.z
        )

        self.isJumping = true
        self:PlayAnimation("Jump_Start")
    end

    if self.isJumping then
        if not isGrounded then
            if self.cb.velocity.y <= 0 then
                self:PlayAnimation("Jump_Loop")
            end
        elseif not self.wasGrounded then
            self.isJumping = false
            self:PlayAnimation("Jump_Land")
        end
    elseif isGrounded then
    local isLanding = (self.currentAnimation == "Jump_Land") and self.animator.isPlaying

    if hasInput then
        if isSprinting then
            self:PlayAnimation("Sprint_Loop")
        else
            self:PlayAnimation("Walk_Loop")
        end
    else
        self:PlayAnimation("Idle_Loop")
    end
end

    self.wasGrounded = isGrounded
end

return TPSCharacter