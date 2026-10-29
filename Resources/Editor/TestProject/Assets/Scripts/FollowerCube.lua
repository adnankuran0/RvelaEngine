---@class FollowerCube : ScriptInstance
---@field agent NavMeshAgentComponent?
---@field target Entity?
---@field targetName string
---@field followSpeed number
---@field stoppingDist number
FollowerCube = {
    className = "FollowerCube"
}

FollowerCube.properties = {
    targetName = "Character",
    followSpeed = 3.0,
    stoppingDist = 1.2
}

function FollowerCube:OnCreate()
    self.agent = self.entity:GetComponent("NavMeshAgent")
    if self.agent then
        self.agent.speed = self.followSpeed or 3.0
        self.agent.stoppingDistance = self.stoppingDist or 1.2
    end
end

function FollowerCube:OnStart()
    self.target = self.scene:FindEntityByName(self.targetName or "Character")
    if not self.target or not self.target:IsValid() then
        self.target = self.scene:FindEntityByName("TPSCharacter")
    end
end

function FollowerCube:OnUpdate(dt)
    if not self.agent then
        self.agent = self.entity:GetComponent("NavMeshAgent")
        if not self.agent then return end
    end

    if not self.target or not self.target:IsValid() then
        self.target = self.scene:FindEntityByName(self.targetName or "Character")
        if not self.target or not self.target:IsValid() then
            self.target = self.scene:FindEntityByName("TPSCharacter")
        end
        return
    end

    local targetTransform = self.target:GetComponent("Transform")
    if targetTransform then
        self.agent:SetDestination(targetTransform.position)
    end
end

return FollowerCube
