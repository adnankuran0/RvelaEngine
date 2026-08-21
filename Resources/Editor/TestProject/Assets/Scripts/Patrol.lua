Patrol = {}

function Patrol:OnCreate()
    self.transform = self.entity:GetComponent("TransformComponent")

    self.state = "idle"
    self.timer = 0
    self.speed = 0
    self.direction = 1
end

function Patrol:OnUpdate(dt)
    self.timer = self.timer - dt

    if self.timer <= 0 then
        if self.state == "idle" then
            self.state = "moving"
            self.timer = math.random(1, 2)
            self.speed = math.random(1, 2)
            self.direction = math.random(0, 1) == 0 and -1 or 1
        else
            self.state = "idle"
            self.timer = math.random(1, 4)
        end
    end

    if self.state == "moving" then
        local position = self.transform.position
        position.x = position.x + self.speed * self.direction * dt
        self.transform.position = position
    end
end

return Patrol