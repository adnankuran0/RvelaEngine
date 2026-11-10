# Lua Scripting Complete Reference

Complete API and usage reference for RvelaEngine Lua scripting (`.lua`).

---

## 1. Script Architecture & Lifecycle

### Script Template
```lua
local Player = {}
Player.className = "Player" -- Type identifier for Entity:GetScriptType()

-- Inspector exposed properties
Player.properties = {
    speed     = 10.0,
    jumpForce = { type = "float", default = 5.0, min = 1.0, max = 20.0 },
    tint      = { type = "color", default = Vec4.new(1, 1, 1, 1) },
    target    = { type = "Entity" },
    prefab    = { type = "AssetHandle", default = "00000000-0000-0000-0000-000000000000" }
}

-- Declared signals
Player.signals = { "OnDied", "OnHealthChanged" }

function Player:OnCreate()
    -- Called once when script is instantiated
end

function Player:OnReady()
    -- Called after all scene entities have initialized
    self.transform = self.entity:GetComponent("Transform")
    self.rb        = self.entity:GetComponent("Rigidbody")
end

function Player:OnUpdate(dt)
    -- Frame update (dt in seconds)
end

function Player:OnFixedUpdate(fixedDt)
    -- Physics step update
end

function Player:OnLateUpdate(dt)
    -- Post-update frame pass
end

function Player:OnDestroy()
    -- Cleanup on entity destroy or scene change
end

return Player
```

### Full Callback List

| Callback | Arguments | Description |
| :--- | :--- | :--- |
| `OnCreate()` | - | Initialization before game start |
| `OnReady()` | - | Startup after all entities initialized |
| `OnUpdate(dt)` | `dt: number` | Frame update (seconds) |
| `OnFixedUpdate(fixedDt)` | `fixedDt: number` | Fixed physics tick update |
| `OnLateUpdate(dt)` | `dt: number` | Post-update frame pass |
| `OnEnabled()` | - | Entity active status changed to true |
| `OnDisabled()` | - | Entity active status changed to false |
| `OnDestroy()` | - | Entity deletion or scene teardown |
| `OnScenePaused()` | - | Scene entered pause state |
| `OnSceneResumed()` | - | Scene resumed from pause |
| `OnCollisionEnter(info)` | `CollisionInfo` | Physics collision started (`info.point`, `info.normal`, `info.other`) |
| `OnCollisionStay(info)` | `CollisionInfo` | Physics collision continues |
| `OnCollisionExit(info)` | `CollisionInfo` | Physics collision ended |
| `OnTriggerEnter(info)` | `CollisionInfo` | Entered trigger volume (`info.other`, `info.isTrigger`) |
| `OnTriggerStay(info)` | `CollisionInfo` | Inside trigger volume |
| `OnTriggerExit(info)` | `CollisionInfo` | Exited trigger volume |
| `OnAnimationStarted(clip)` | `clipName: string` | Animation clip playback started |
| `OnAnimationFinished(clip)`| `clipName: string` | Animation clip playback completed |
| `OnAnimationLooped(clip)` | `clipName: string` | Looped animation cycle completed |
| `OnAnimationEvent(src, event, param)` | `src: Entity, name: string, param: string` | Animation timeline event triggered |
| `OnAudioFinished()` | - | AudioEmitter component finished playing |

---

## 2. Window & Application Management

### Window API (`Window`)
```lua
-- Window Dimensions & Position
Window.SetSize(1920, 1080)
local w    = Window.GetWidth()
local h    = Window.GetHeight()
local size = Window.GetSize() -- Returns Vec2
Window.Center()

-- Window Properties
Window.SetTitle("My Rvela Game")
local title = Window.GetTitle()
Window.SetVSync(true)
local vsync = Window.IsVSync()
Window.SetBorderless(false)
Window.SetResizable(true)

-- Window Modes
Window.SetWindowMode(WindowMode.Fullscreen) -- Windowed, Minimized, Maximized, Fullscreen
Window.SetFullscreen(true)
Window.SetMaximized(true)
local isMax = Window.IsMaximized()

-- Icon & Cursor
Window.SetIcon("Assets/Textures/Icon.png")
Window.ResetIcon()
Window.SetStandardCursor(CursorShape.Hand) -- Arrow, IBeam, Crosshair, Hand, HResize, VResize
Window.SetCursorIcon("Assets/Textures/Cursor.png", 0, 0) -- Custom cursor
Window.ResetCursor()
```

### Engine & Application API (`Engine` / `Application`)
```lua
Engine.Quit()          -- Exit application (Alias: Application.Quit(), Engine.Exit())
Engine.SetMaxFPS(144)  -- Cap framerate (0 = uncapped)
local fpsCap = Engine.GetMaxFPS()
```

### Time API (`Time`)
```lua
local dt       = Time.GetDeltaTime()      -- Alias: Time.deltaTime
local fixedDt  = Time.GetFixedDeltaTime()
local fps      = Time.GetFPS()            -- Alias: Time.fps
local timeSec  = Time.time
Time.SetTimeScale(0.5)                    -- Slow motion / pause
local scale    = Time.GetTimeScale()      -- Alias: Time.timeScale
```

---

## 3. Storage & Save System (`Storage` / `SaveSystem`)

Reads and writes persistent files in the project's `Saves/` directory.

```lua
-- Save / Load text (e.g. JSON string)
Storage.SaveText("savegame.json", jsonString)

if Storage.FileExists("savegame.json") then
    local content = Storage.LoadText("savegame.json")
end

Storage.DeleteFile("savegame.json")
local savePath = Storage.GetSaveDirectory()
```

---

## 4. Input & Mouse API

### Keyboard
```lua
Input.IsKeyPressed(Key.W)           -- Held down
Input.IsKeyJustPressed(Key.Space)   -- Pressed this frame
Input.IsKeyJustReleased(Key.Escape) -- Released this frame
```

### Mouse
```lua
Input.IsMouseButtonPressed(MouseButton.Left)
Input.IsMouseButtonJustPressed(MouseButton.Right)
Input.IsMouseButtonJustReleased(MouseButton.Middle)

-- Mouse Coordinates & Delta
local pos    = Input.mousePosition       -- Vec2 (screen x, y)
local delta  = Input.mouseDelta          -- Vec2 (dx, dy)
local scroll = Input.mouseScroll         -- Vec2 (scroll x, y)
local dx     = Input.GetMouseDeltaX()
local dy     = Input.GetMouseDeltaY()

-- Mouse Capture / Cursor Mode
Input.SetMouseMode(MouseMode.CAPTURED)   -- VISIBLE, HIDDEN, CAPTURED
local overUI = Input.IsMouseOverUI()
```

---

## 5. Entity & Hierarchy

### Entity Properties & Methods
```lua
-- Identification & Status
local valid = self.entity:IsValid()
local name  = self.entity.name          -- or entity:GetName()
self.entity.name = "Player"
self.entity.active = true               -- or entity:SetActive(bool)
local activeInTree = self.entity:IsActive()
local selfActive   = self.entity:IsSelfActive()
local uuidStr      = self.entity:GetUUID()

-- Hierarchy
self.entity:SetParent(parentEntity)     -- Pass nil to unparent
local parent = self.entity:GetParent()
local child  = self.entity:FindChild("Gun")

-- Script Inter-Communication & Type Checking
local scriptType = target:GetScriptType() -- Returns script's `className` field (e.g. "Enemy")
if scriptType == "Enemy" and target:HasMethod("TakeDamage") then
    target:CallMethod("TakeDamage", 25, self.entity)
end

-- Destruction
self.entity:Destroy()                   -- Queues entity destruction
```

### Signals
```lua
-- Connect signal from source to target method
local connId = sourceEntity:ConnectSignal("OnDied", self.entity, "HandleTargetDeath")
sourceEntity:DisconnectSignal(connId)

-- Emit signal from own script
self:EmitSignal("OnDied")
self:EmitSignal("OnHealthChanged", 75, 100)
```

---

## 6. Scene & Asset Management

### Scene API (`Scene`)
```lua
-- Spawning prefabs
local instance = self.scene:Instantiate(prefabHandle, positionVec3, rotationQuatOrVec3, parentEntity)

-- Entities
local enemy = self.scene:FindEntityByName("Boss")
local empty = self.scene:CreateEntity("Waypoint")
self.scene:DestroyEntity(enemy)

-- Autoload singletons
local gameManager = self.scene:GetAutoload("GameManager")
```

### SceneManager API (`SceneManager`)
```lua
-- Synchronous scene change
SceneManager.ChangeScene("Assets/Scenes/Level1.rscene")

-- Asynchronous scene loading
local taskId = SceneManager.LoadSceneAsync("Assets/Scenes/Level2.rscene", false)
local progress = SceneManager.GetLoadProgress(taskId) -- [0.0 - 1.0]
local status   = SceneManager.GetLoadStatus(taskId)   -- "Loading", "Loaded", "Failed"

if SceneManager.IsSceneLoaded(taskId) then
    SceneManager.ChangeToLoadedScene(taskId)
end

SceneManager.CancelAsyncLoad(taskId)
SceneManager.UnloadUnusedAssets()
```

### AssetManager API (`AssetManager` / `Assets`)
```lua
local handle = AssetManager.GetByPath("Assets/Prefabs/Bullet.rprefab")
local path   = AssetManager.GetPath(handle)
local exists = AssetManager.Exists(handle)
local valid  = handle:IsValid()
```

---

## 7. Physics & Colliders

### Raycasting & Shapecasting (`Physics`)
```lua
-- Raycast using table syntax:
local hit = Physics.Raycast({
    origin      = self.transform.worldPosition, -- required: Vec3
    dir         = self.transform.forward,       -- required: Vec3
    maxDistance = 50.0,                         -- optional (default: 1000.0)
    hitInside   = false,                        -- optional (default: false)
    ignore      = self.character,               -- optional: Entity or { Entity, Entity }
    layerMask   = 0xFFFFFFFF                    -- optional: integer mask
})

-- Or using RaycastOptions instance:
local opts = RaycastOptions.new(origin, dir, 50.0)
opts.ignore = self.character
local hit = Physics.Raycast(opts)
if hit.hit then
    local hitEntity = hit.entity     -- Entity
    local hitPoint  = hit.point      -- Vec3
    local hitNormal = hit.normal     -- Vec3
    local hitDist   = hit.distance   -- number
end

local allHits = Physics.RaycastAll({
    origin      = origin,
    dir         = dir,
    maxDistance = 100.0,
    ignore      = self.character
})

-- Shape Casts
local boxHit  = Physics.BoxCast(origin, halfExtent, rot, dir, maxDist)
local capHit  = Physics.CapsuleCast(origin, radius, halfHeight, rot, dir, maxDist)

-- Layer masks
local mask = Physics.GetLayerMask("Default", "Enemies")
```

### Rigidbody Component (`"Rigidbody"`)
```lua
local rb = self.entity:GetComponent("Rigidbody")

-- Properties
rb.mass            = 5.0
rb.motionType      = MotionType.Dynamic -- Static, Kinematic, Dynamic
rb.velocity        = Vec3.new(0, 0, 5)
rb.angularVelocity = Vec3.new(0, 0, 0)
rb.friction        = 0.5
rb.restitution     = 0.2
rb.gravityFactor   = 1.0
rb.isSensor        = false
rb.collisionLayer  = 1
rb.collisionMask   = 0xFFFFFFFF

-- Forces & Impulses
rb:AddForce(Vec3.new(0, 100, 0))
rb:AddImpulse(Vec3.new(0, 10, 0))
rb:AddTorque(Vec3.new(0, 50, 0))
rb:AddAngularImpulse(Vec3.new(0, 5, 0))
rb:AddLinearVelocity(Vec3.new(0, 2, 0))
rb:MoveKinematic(targetPos, targetRot, dt)
```

### CharacterBody Component (`"CharacterBody"`)
```lua
local cb = self.entity:GetComponent("CharacterBody")

cb.velocity      = Vec3.new(moveX, velocityY, moveZ)
cb.maxSlopeAngle = 45.0

-- Ground checks
local grounded   = cb.isGrounded
local inAir      = cb.isInAir
local steep      = cb.isOnSteepGround
local normal     = cb.groundNormal
local groundPos  = cb.groundPosition
```

### Colliders (`"BoxCollider"`, `"SphereCollider"`, `"CapsuleCollider"`)
```lua
local box = self.entity:GetComponent("BoxCollider")
box.size   = Vec3.new(1, 2, 1)
box.offset = Vec3.new(0, 1, 0)

local sphere = self.entity:GetComponent("SphereCollider")
sphere.radius = 0.5

local cap = self.entity:GetComponent("CapsuleCollider")
cap.radius     = 0.5
cap.halfHeight = 1.0
```

---

## 8. Navigation & NavMesh (`NavMesh`)

```lua
-- Global Pathfinding
local pathPoints = NavMesh.CalculatePath(startPos, endPos)
local snappedPos = NavMesh.SamplePosition(targetPos, Vec3.new(2, 4, 2))
local hitPos     = NavMesh.Raycast(startPos, endPos)

-- NavMesh Agent Component
local agent = self.entity:GetComponent("NavMeshAgent")
agent.speed            = 6.0
agent.stoppingDistance = 0.5
agent.acceleration     = 12.0
agent.updatePosition   = true
agent.updateRotation   = true

agent:SetDestination(targetPos)
local hasPath  = agent.hasPath
local moving   = agent.isMoving
local reached  = agent.reachedDestination
local distLeft = agent.remainingDistance
agent:Stop()
agent:Resume()
agent:ResetPath()
```

---

## 9. Audio System (`Audio` & `"AudioEmitter"`)

```lua
local emitter = self.entity:GetComponent("AudioEmitter")

-- Playback
emitter:Play()
emitter:Pause()
emitter:Resume()
emitter:Stop()
local playing = emitter:IsPlaying()

-- Audio Properties
emitter.volume           = 0.8
emitter.pitch            = 1.0
emitter.loop             = true
emitter.spatial          = true
emitter.attenuationModel = AttenuationModel.Inverse -- None, Inverse, Linear, Exponential
emitter.minDistance      = 1.0
emitter.maxDistance      = 50.0
emitter.rolloff          = 1.0

-- Bus Control
local sfxBus = Audio.GetBusID("SFX")
Audio.SetBusVolume(sfxBus, 0.7)

-- Audio Effects (Emitter)
local filter = emitter:GetFilterEffect() -- Gets first filter if attached
if not filter then
    filter = emitter:AddEffect(AudioEffectType.Filter)
end
filter.type      = FilterType.LowPass -- LowPass, HighPass, BandPass, Notch
filter.cutoff    = 2500.0            -- Cutoff frequency in Hz (20..20000)
filter.resonance = 1.0               -- Q / Resonance (0.1..10.0)
filter.enabled   = true

-- Other effect methods:
-- emitter:GetEffect(index)               -- 0-based index
-- emitter:GetEffectByType(effectType)    -- AudioEffectType: Filter, Reverb, Delay, Distortion, Limiter, Equalizer
-- emitter:GetEffects()                   -- 1-indexed Lua array of all effects
-- emitter:RemoveEffect(index)            -- Remove by 0-based index
-- emitter:ClearEffects()                 -- Remove all

-- Bus Effects
local busFilter = Audio.GetBusFilterEffect(sfxBus)
if busFilter then
    busFilter.cutoff = 1000.0
end
-- Audio.AddBusEffect(sfxBus, AudioEffectType.Filter)
-- Audio.GetBusEffects(sfxBus)
```

---

## 10. Animation System (`"Animator"`)

```lua
local anim = self.entity:GetComponent("Animator")

anim:Play("Walk", 0.2) -- Clip name, blend time in seconds
anim:Pause()
anim:Stop()
anim.speed = 1.5

local current = anim.currentClipName
local isPlay  = anim.isPlaying
```

---

## 11. UI Components

### RectTransform (`"RectTransform"`)
```lua
local rt = self.entity:GetComponent("RectTransform")
rt.position      = Vec2.new(100, 50)
rt.size          = Vec2.new(200, 60)
rt.anchorMin     = Vec2.new(0.5, 0.5)
rt.anchorMax     = Vec2.new(0.5, 0.5)
rt.pivot         = Vec2.new(0.5, 0.5)
rt.rotation      = 0.0
rt.scale         = Vec2.new(1, 1)
rt.raycastTarget = true
```

### UI Elements
```lua
-- Image
local img = self.entity:GetComponent("UIImage")
img.color = Vec4.new(1, 1, 1, 0.8)
img:SetTexture(textureUUID)

-- Text
local txt = self.entity:GetComponent("UIText")
txt.text     = "Score: 100"
txt.fontSize = 24.0
txt.color    = Vec4.new(1, 1, 0, 1)

-- Button
local btn = self.entity:GetComponent("UIButton")
btn.interactable = true
btn:OnClick(function()
    Log.Info("Button clicked!")
end)

-- Slider
local slider = self.entity:GetComponent("UISlider")
slider.minValue = 0.0
slider.maxValue = 100.0
slider.value    = 50.0
slider:OnValueChanged(function(val)
    Log.Info("Slider changed: " .. tostring(val))
end)
```

---

## 12. Material Component & Custom Shaders

Control material properties and custom shader uniforms at runtime:

```lua
local mat = self.entity:GetComponent("Material")

-- PBR Parameters
mat.albedoColor       = Vec4.new(1, 0, 0, 1)
mat.roughness         = 0.2
mat.metallic          = 0.8
mat.emissiveColor     = Vec3.new(0, 1, 0)
mat.emissiveIntensity = 2.0
mat.hasOutline        = true
mat.outlineColor      = Vec4.new(1, 1, 0, 1)

-- Custom Shader Uniforms
if mat:HasCustomShader() then
    mat:SetShaderFloat("u_Speed", 2.5)
    mat:SetShaderVec3("u_Color", Vec3.new(0.2, 0.5, 1.0))
    mat:SetShaderColor("u_Tint", Vec4.new(1, 1, 1, 1))
    mat:SetShaderBool("u_Active", true)
end
```

---

## 13. Transform, Camera & Lighting

### Transform Component (`"Transform"`)
```lua
local t = self.entity:GetComponent("Transform")

-- Position & Vectors
t.position       = Vec3.new(0, 5, 0)
local worldPos   = t.worldPosition
local fwd        = t.forward
local up         = t.up
local right      = t.right

-- Rotation
t.eulerRotation  = Vec3.new(0, 90, 0) -- Degrees
t.rotation       = Quat.Euler(0, 90, 0)

-- Helpers
t:Translate(Vec3.new(0, 0, 1))
t:Rotate(45.0, Vec3.UP)
t:LookAt(targetPos, Vec3.UP)
```

### Camera Component (`"Camera"`)
```lua
local cam = self.entity:GetComponent("Camera")
cam.fov = 60.0
```

### Lights (`"DirectionalLight"`, `"PointLight"`, `"SpotLight"`)
```lua
local light = self.entity:GetComponent("PointLight")
light.color       = Vec3.new(1, 0.8, 0.6)
light.intensity   = 10.0
light.radius      = 15.0
light.castShadows = true
```

---

## 14. Timers, Coroutines & Tweens

```lua
-- Timers
local timerId = self:StartTimer(3.0, function(self)
    Log.Info("3 seconds elapsed")
end)
self:CancelTimer(timerId)

self:StartRepeatingTimer(1.0, function(self)
    -- Ticks every second
end)

-- Coroutines with await
self:StartCoroutine(function(self)
    Log.Info("Spawn wave 1")
    await(2.0)
    Log.Info("Spawn wave 2 after 2.0s")
end)

-- Tweening
self:TweenPosition(Vec3.new(0, 5, 0), 1.5)
    :SetEase(Ease.OutBounce)
    :SetDelay(0.2)
    :OnComplete(function()
        Log.Info("Arrived!")
    end)

-- Tweening arbitrary numeric/vector values
Tween.Value(0.0, 1.0, 2.0, function(val)
    mat.roughness = val
end)
```

---

## 15. Debug Drawing & Logging

```lua
-- Console Logs
Log.Info("Player health: ", 100)
Log.Warn("Low ammo!")
Log.Error("Failed to find item!")

-- Debug Shapes (Rendered in viewport for 1 frame)
Debug.DrawLine(posA, posB, Vec4.new(1, 0, 0, 1))
Debug.DrawBox(minVec3, maxVec3, Vec4.new(0, 1, 0, 1))
Debug.DrawTriangle(p1, p2, p3, Vec4.new(0, 0, 1, 1))
```
