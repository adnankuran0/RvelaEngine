---@class Ball : ScriptInstance
---@field speed number
---@field destroySecons number
---@field minImpactSpeed number
---@field wallParticlePrefab AssetHandle
local Ball = {}

local WALL_PARTICLE_UUID = "1c0b2d11-6b85-4e49-94d9-b4a4030d598d"

Ball.properties = {
    speed = 100.0,
    destroySecons = 2,
    minImpactSpeed = 2.0,
    wallParticlePrefab = { type = "AssetHandle", default = WALL_PARTICLE_UUID }
}

function Ball:OnCreate()
    self.spawnedParticles = {}
    self:StartTimer(self.destroySecons, function(self)
        self.entity:Destroy()
    end)
end

function Ball:OnReady()
    if not self.spawnedParticles then
        self.spawnedParticles = {}
    end
    self.speed = self.speed or Ball.properties.speed or 100.0
    self.minImpactSpeed = self.minImpactSpeed or Ball.properties.minImpactSpeed or 2.0
    self.transform = self.entity:GetComponent("Transform")
    self.rb = self.entity:GetComponent("Rigidbody")
    if self.rb and self.rb:IsValid() and self.transform and self.transform:IsValid() then
        self.rb:AddImpulse(self.transform.forward * self.speed)
        self.lastVelocity = self.rb.velocity
    end

    if not self.wallParticlePrefab or (type(self.wallParticlePrefab) == "userdata" and not self.wallParticlePrefab:IsValid()) then
        self.wallParticlePrefab = AssetHandle.FromString(WALL_PARTICLE_UUID)
    end
end

function Ball:OnFixedUpdate(fixedDt)
    if self.rb and self.rb:IsValid() then
        self.lastVelocity = self.rb.velocity
    end
end

function Ball:OnDestroy()
    if self.spawnedParticles then
        for _, particle in ipairs(self.spawnedParticles) do
            if particle and particle:IsValid() then
                particle:Destroy()
            end
        end
        self.spawnedParticles = {}
    end
end

local function GetRotationFromNormal(normal)
    local n = normal:Normalized()
    local pitch = math.deg(math.asin(math.max(-1.0, math.min(1.0, n.y))))
    local yaw = math.deg(math.atan2(-n.x, -n.z))
    return Vec3.new(pitch, yaw, 0.0)
end

---@param collision CollisionInfo
function Ball:OnCollisionEnter(collision)
    local vel = (self.lastVelocity and self.lastVelocity:LengthSq() > 0.001) and self.lastVelocity or (self.rb and self.rb.velocity)
    local impactSpeed = 0.0
    if vel then
        impactSpeed = math.abs(vel:Dot(collision.normal))
        if impactSpeed < 0.001 then
            impactSpeed = vel:Length()
        end
    end

    local threshold = self.minImpactSpeed or Ball.properties.minImpactSpeed or 2.0
    if impactSpeed < threshold then
        return
    end

    local prefab = self.wallParticlePrefab
    if not prefab or (type(prefab) == "userdata" and not prefab:IsValid()) then
        prefab = AssetHandle.FromString(WALL_PARTICLE_UUID)
    end

    if self.scene and prefab then
        local spawnRot = GetRotationFromNormal(collision.normal)

        local particleEntity = self.scene:Instantiate(prefab, collision.point, spawnRot)
        if particleEntity and particleEntity:IsValid() then
            if not self.spawnedParticles then
                self.spawnedParticles = {}
            end
            table.insert(self.spawnedParticles, particleEntity)

            local pTransform = particleEntity:GetComponent("Transform")
            if pTransform and pTransform:IsValid() then
                pTransform.position = collision.point
                pTransform.eulerRotation = spawnRot
            end

            local emitter = particleEntity:GetComponent("ParticleEmitter")
            if emitter and emitter:IsValid() then
                emitter.emitting = true
                local audio = particleEntity:GetComponent("AudioEmitter")
                if audio and audio:IsValid() then
                    audio.pitch = 0.9 + math.random() * 0.20
                    audio:Play()
                end
            end
        end
    end
end

return Ball

