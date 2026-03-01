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
    local x, z = 0, 0

    if Input.IsKeyPressed(KeyCode.LeftShift) then
        speed = sprint_speed
    end

    if Input.IsKeyPressed(KeyCode.W) then
        z = z - 1
    end
    if Input.IsKeyPressed(KeyCode.S) then
        z = z + 1
    end
    if Input.IsKeyPressed(KeyCode.A) then
        x = x - 1
    end
    if Input.IsKeyPressed(KeyCode.D) then
        x = x + 1
    end

    if x ~= 0 or z ~= 0 then
        transform:Translate(vec3.new(x * speed * dt, 0, z * speed * dt))
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