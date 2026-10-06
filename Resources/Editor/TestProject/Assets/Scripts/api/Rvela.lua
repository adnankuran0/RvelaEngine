---@meta RvelaEngine Lua API Definitions
--- This file provides full IntelliSense, Autocomplete, and Type Checking for RvelaEngine in VS Code.
--- Generated for EmmyLua / Lua Language Server (LuaLS / sumneko.lua).

-------------------------------------------------------------------------------
-- MATH TYPES
-------------------------------------------------------------------------------

---@class Vec2
---@field x number
---@field y number
---@field ZERO Vec2 (0, 0)
---@field UP Vec2 (0, 1)
---@field RIGHT Vec2 (1, 0)
---@operator add(Vec2): Vec2
---@operator sub(Vec2): Vec2
---@operator mul(number): Vec2
---@operator div(number): Vec2
---@operator unm: Vec2
local Vec2 = {}

---Creates a new Vec2.
---@overload fun(): Vec2
---@overload fun(x: number, y: number): Vec2
---@param x? number
---@param y? number
---@return Vec2
function Vec2.new(x, y) end

---Returns the length (magnitude) of the vector.
---@return number
function Vec2:Length() end

---Returns the squared length of the vector.
---@return number
function Vec2:LengthSq() end

---Returns a normalized copy of the vector.
---@return Vec2
function Vec2:Normalized() end

---Calculates the dot product with another vector.
---@param other Vec2
---@return number
function Vec2:Dot(other) end

---Calculates the distance to another vector.
---@param other Vec2
---@return number
function Vec2:Distance(other) end

---Linearly interpolates between this vector and target.
---@param target Vec2
---@param t number
---@return Vec2
function Vec2:Lerp(target, t) end

_G.Vec2 = Vec2


---@class Vec3
---@field x number
---@field y number
---@field z number
---@field ZERO Vec3 (0, 0, 0)
---@field UP Vec3 (0, 1, 0)
---@field RIGHT Vec3 (1, 0, 0)
---@field FORWARD Vec3 (0, 0, -1)
---@operator add(Vec3): Vec3
---@operator sub(Vec3): Vec3
---@operator mul(number): Vec3
---@operator div(number): Vec3
---@operator unm: Vec3
local Vec3 = {}

---Creates a new Vec3.
---@overload fun(): Vec3
---@overload fun(x: number, y: number, z: number): Vec3
---@param x? number
---@param y? number
---@param z? number
---@return Vec3
function Vec3.new(x, y, z) end

---Returns the length of the vector.
---@return number
function Vec3:Length() end

---Returns the squared length of the vector.
---@return number
function Vec3:LengthSq() end

---Returns a normalized copy of the vector.
---@return Vec3
function Vec3:Normalized() end

---Calculates the dot product with another vector.
---@param other Vec3
---@return number
function Vec3:Dot(other) end

---Calculates the cross product with another vector.
---@param other Vec3
---@return Vec3
function Vec3:Cross(other) end

---Calculates the distance to another vector.
---@param other Vec3
---@return number
function Vec3:Distance(other) end

---Linearly interpolates between this vector and target.
---@param target Vec3
---@param t number
---@return Vec3
function Vec3:Lerp(target, t) end

---Reflects this vector off a surface normal.
---@param normal Vec3
---@return Vec3
function Vec3:Reflect(normal) end

_G.Vec3 = Vec3


---@class Vec4
---@field x number
---@field y number
---@field z number
---@field w number
---@field r number
---@field g number
---@field b number
---@field a number
---@field ZERO Vec4
---@field ONE Vec4
---@operator add(Vec4): Vec4
---@operator sub(Vec4): Vec4
---@operator mul(number|Vec4): Vec4
---@operator div(number): Vec4
---@operator unm: Vec4
local Vec4 = {}

---Creates a new Vec4 / Color.
---@overload fun(): Vec4
---@overload fun(scalar: number): Vec4
---@overload fun(x: number, y: number, z: number, w: number): Vec4
---@param x? number
---@param y? number
---@param z? number
---@param w? number
---@return Vec4
function Vec4.new(x, y, z, w) end

---Returns the length of the vector.
---@return number
function Vec4:Length() end

---Returns the squared length of the vector.
---@return number
function Vec4:LengthSq() end

---Returns a normalized copy of the vector.
---@return Vec4
function Vec4:Normalized() end

---Calculates the dot product.
---@param other Vec4
---@return number
function Vec4:Dot(other) end

---Calculates distance to another vector.
---@param other Vec4
---@return number
function Vec4:Distance(other) end

---Linearly interpolates between this vector and target.
---@param target Vec4
---@param t number
---@return Vec4
function Vec4:Lerp(target, t) end

_G.Vec4 = Vec4
---@class Color : Vec4
_G.Color = Vec4


---@class Quat
---@operator mul(Quat|Vec3): Quat|Vec3
local Quat = {}

---Creates a new Quaternion.
---@overload fun(): Quat
---@overload fun(w: number, x: number, y: number, z: number): Quat
---@param w? number
---@param x? number
---@param y? number
---@param z? number
---@return Quat
function Quat.new(w, x, y, z) end

---Returns a normalized copy of the quaternion.
---@return Quat
function Quat:Normalized() end

---Converts quaternion to Euler angles (in radians: pitch, yaw, roll).
---@return Vec3
function Quat:ToEuler() end

---Rotates a vector by this quaternion.
---@param v Vec3
---@return Vec3
function Quat:Rotate(v) end

_G.Quat = Quat


---@class Math
Math = {}

---Clamps a value between min and max.
---@param v number
---@param min number
---@param max number
---@return number
function Math.Clamp(v, min, max) end

---Linearly interpolates between a and b by t.
---@param a number
---@param b number
---@param t number
---@return number
function Math.Lerp(a, b, t) end

---Converts degrees to radians.
---@param degrees number
---@return number
function Math.Radians(degrees) end

---Converts radians to degrees.
---@param radians number
---@return number
function Math.Degrees(radians) end

_G.Math = Math


-------------------------------------------------------------------------------
-- ASSET TYPES
-------------------------------------------------------------------------------

---@class AssetHandle
local AssetHandle = {}

---Creates a new AssetHandle.
---@overload fun(): AssetHandle
---@overload fun(uuidStr: string): AssetHandle
---@param uuidStr? string
---@return AssetHandle
function AssetHandle.new(uuidStr) end

---Returns the string representation of the UUID.
---@return string
function AssetHandle:ToString() end

---Checks if this asset handle is valid.
---@return boolean
function AssetHandle:IsValid() end

---Checks if this asset handle is empty/invalid.
---@return boolean
function AssetHandle:IsEmpty() end

---Parses an AssetHandle from a UUID string.
---@param str string
---@return AssetHandle
function AssetHandle.FromString(str) end

---Returns an invalid AssetHandle.
---@return AssetHandle
function AssetHandle.Invalid() end

_G.AssetHandle = AssetHandle


---@class AssetManager
AssetManager = {}

---Gets asset UUID by project-relative path (e.g. "Assets/Textures/stone.png").
---@param path string
---@return AssetHandle
function AssetManager.GetByPath(path) end

---Gets asset filesystem path from UUID handle.
---@param handle AssetHandle
---@return string
function AssetManager.GetPath(handle) end

---Checks if an asset with given UUID handle exists in registry.
---@param handle AssetHandle
---@return boolean
function AssetManager.Exists(handle) end

---Resolves an AssetHandle from an AssetHandle or UUID string.
---@param obj AssetHandle|string
---@return AssetHandle
function AssetManager.GetHandle(obj) end

_G.AssetManager = AssetManager
_G.Assets = AssetManager


-------------------------------------------------------------------------------
-- INPUT SYSTEM
-------------------------------------------------------------------------------

---@enum KeyCode
KeyCode = {
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    D0 = 48, D1 = 49, D2 = 50, D3 = 51, D4 = 52,
    D5 = 53, D6 = 54, D7 = 55, D8 = 56, D9 = 57,
    Semicolon = 59,
    Equal = 61,
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    World1 = 161,
    World2 = 162,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    ScrollLock = 281,
    NumLock = 282,
    PrintScreen = 283,
    Pause = 284,
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    F13 = 302, F14 = 303, F15 = 304, F16 = 305, F17 = 306,
    F18 = 307, F19 = 308, F20 = 309, F21 = 310, F22 = 311,
    F23 = 312, F24 = 313, F25 = 314,
    KP0 = 320, KP1 = 321, KP2 = 322, KP3 = 323, KP4 = 324,
    KP5 = 325, KP6 = 326, KP7 = 327, KP8 = 328, KP9 = 329,
    KPDecimal = 330,
    KPDivide = 331,
    KPMultiply = 332,
    KPSubtract = 333,
    KPAdd = 334,
    KPEnter = 335,
    KPEqual = 336,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
    Menu = 348,
}
_G.KeyCode = KeyCode

---@enum MouseCode
MouseCode = {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Button6 = 6,
    Button7 = 7,
    ButtonLast = 7,
    Left = 0,
    Right = 1,
    Middle = 2,
}
_G.MouseCode = MouseCode
_G.MouseButton = MouseCode

---@enum MouseMode
MouseMode = {
    VISIBLE = 0,
    HIDDEN = 1,
    CAPTURED = 2,
}
_G.MouseMode = MouseMode

---@class Input
Input = {}

---Checks if a keyboard key is currently held down.
---@param key KeyCode|number
---@return boolean
function Input.IsKeyPressed(key) end

---Checks if a keyboard key was pressed this frame.
---@param key KeyCode|number
---@return boolean
function Input.IsKeyJustPressed(key) end

---Checks if a keyboard key was released this frame.
---@param key KeyCode|number
---@return boolean
function Input.IsKeyJustReleased(key) end

---Checks if a mouse button is currently held down.
---@param button MouseCode|number
---@return boolean
function Input.IsMouseButtonPressed(button) end

---Checks if a mouse button was clicked this frame.
---@param button MouseCode|number
---@return boolean
function Input.IsMouseButtonJustPressed(button) end

---Checks if a mouse button was released this frame.
---@param button MouseCode|number
---@return boolean
function Input.IsMouseButtonJustReleased(button) end

---Returns the current cursor position in screen pixels.
---@return Vec2
function Input.GetMousePosition() end

---Sets the cursor mode (MouseMode.VISIBLE, MouseMode.HIDDEN, MouseMode.CAPTURED).
---@param mode MouseMode|number
function Input.SetMouseMode(mode) end

---Checks if the mouse is currently hovering over an interactive UI element.
---@return boolean
function Input.IsMouseOverUI() end

_G.Input = Input


-------------------------------------------------------------------------------
-- PHYSICS SYSTEM
-------------------------------------------------------------------------------

---@enum MotionType
MotionType = {
    Static = 0,
    Kinematic = 1,
    Dynamic = 2,
}
_G.MotionType = MotionType

---@class CollisionInfo
---@field point Vec3 Collision contact point
---@field normal Vec3 Surface contact normal
---@field other Entity The other entity collided with
---@field isTrigger boolean Whether this contact came from a sensor/trigger body

---@class RaycastResult
---@field hit boolean Whether the ray hit something
---@field distance number Distance from ray origin to hit point
---@field point Vec3 Contact point in world space
---@field normal Vec3 Surface normal at contact point
---@field entity Entity The entity that was hit

---@class Physics
Physics = {}

---Casts a ray into the physics world.
---@param origin Vec3 Ray starting position
---@param dir Vec3 Ray direction vector (should be normalized)
---@param maxDistance number Maximum ray distance
---@param hitInside boolean Whether to detect hits if starting inside a collider
---@return RaycastResult
function Physics.Raycast(origin, dir, maxDistance, hitInside) end

_G.Physics = Physics


-------------------------------------------------------------------------------
-- AUDIO SYSTEM
-------------------------------------------------------------------------------

---@enum AttenuationModel
AttenuationModel = {
    None = 0,
    Inverse = 1,
    Linear = 2,
    Exponential = 3,
}
_G.AttenuationModel = AttenuationModel

---@class Audio
Audio = {}

---Sets the volume of an audio bus.
---@param busID integer
---@param volume number
function Audio.SetBusVolume(busID, volume) end

---Gets the volume of an audio bus.
---@param busID integer
---@return number
function Audio.GetBusVolume(busID) end

---Gets the numeric ID of an audio bus by name.
---@param name string
---@return integer
function Audio.GetBusID(name) end

---Sets the parent bus for routing.
---@param busID integer
---@param parentID integer
function Audio.SetParentBus(busID, parentID) end

_G.Audio = Audio


-------------------------------------------------------------------------------
-- ANIMATION SYSTEM
-------------------------------------------------------------------------------

---@enum LoopMode
LoopMode = {
    None = 0,
    Linear = 1,
    PingPong = 2,
}
_G.LoopMode = LoopMode

---@enum EaseType
EaseType = {
    Linear = 0,
    EaseIn = 1,
    EaseOut = 2,
    EaseInOut = 3,
    EaseOutIn = 4,
    Zero = 5,
}
_G.EaseType = EaseType


-------------------------------------------------------------------------------
-- PARTICLE SYSTEM
-------------------------------------------------------------------------------

---@enum ParticleEmitterShape
ParticleEmitterShape = {
    Point = 0,
    Sphere = 1,
    SphereSurface = 2,
    Box = 3,
}
_G.ParticleEmitterShape = ParticleEmitterShape


-------------------------------------------------------------------------------
-- COMPONENTS
-------------------------------------------------------------------------------

---@class ComponentHandle
local ComponentHandle = {}
---@return boolean
function ComponentHandle:IsValid() end

---@class TransformComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field position Vec3 Local position
---@field worldPosition Vec3 World position (read-only)
---@field rotation Quat Local rotation quaternion
---@field worldRotation Quat World rotation quaternion (read-only)
---@field eulerRotation Vec3 Local Euler angles in degrees (pitch, yaw, roll)
---@field scale Vec3 Local scale
---@field worldScale Vec3 World scale (read-only)
---@field forward Vec3 Forward direction vector (read-only)
---@field up Vec3 Up direction vector (read-only)
---@field right Vec3 Right direction vector (read-only)
local TransformComponent = {}
---Translates local position by vector.
---@param delta Vec3
function TransformComponent:Translate(delta) end
---Rotates transform to point towards target position.
---@param target Vec3
---@param up Vec3
function TransformComponent:LookAt(target, up) end

---@class CameraComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field fov number Field of view in degrees

---@class DirectionalLightComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field color Vec3 Light color (RGB)
---@field intensity number Light intensity
---@field shadowBias number Shadow depth bias
---@field blurRadius number Shadow PCF blur radius
---@field castShadows boolean Whether light casts shadows
---@field reverseCullFace boolean Shadow reverse culling

---@class PointLightComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field color Vec3 Light color (RGB)
---@field intensity number Light intensity
---@field radius number Light radius in world units
---@field falloff number Attenuation falloff
---@field castShadows boolean Whether point light casts shadows
---@field reverseCullFace boolean Shadow reverse culling
---@field shadowBias number Shadow depth bias
---@field blurRadius number Shadow blur radius

---@class MeshRendererComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field castShadow boolean Whether mesh casts shadow

---@class MaterialComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field albedoColor Vec4 Base albedo color (RGBA)
---@field emissiveColor Vec3 Emissive color (RGB)
---@field emissiveIntensity number Emissive brightness multiplier
---@field metallic number Metallic factor [0..1]
---@field specular number Specular factor [0..1]
---@field roughness number Roughness factor [0..1]
---@field ao number Ambient occlusion factor [0..1]
---@field normalScale number Normal map intensity scale
---@field heightScale number Parallax displacement height scale
---@field uvScale Vec2 UV tiling scale
---@field uvOffset Vec2 UV scroll offset

---@class RigidbodyComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field motionType MotionType Static, Kinematic, or Dynamic
---@field mass number Body mass in kg
---@field velocity Vec3 Linear velocity vector
---@field maxVelocity number Max linear velocity cap
---@field angularVelocity Vec3 Angular velocity vector
---@field maxAngularVelocity number Max angular velocity cap
---@field friction number Friction coefficient
---@field gravityFactor number Gravity multiplier
---@field restitution number Bounciness [0..1]
---@field isSensor boolean Whether this body acts as a trigger/sensor
---@field centerOfMass Vec3 Center of mass position (read-only)
---@field position Vec3 Position
---@field rotation Quat Rotation quaternion
local RigidbodyComponent = {}
---Moves kinematic body towards position and rotation over delta time.
---@param pos Vec3
---@param rot Quat
---@param dt number
function RigidbodyComponent:MoveKinematic(pos, rot, dt) end
---Applies a continuous force to the center of mass.
---@param force Vec3
function RigidbodyComponent:AddForce(force) end
---Applies an instant impulse to the body.
---@param impulse Vec3
function RigidbodyComponent:AddImpulse(impulse) end
---Applies continuous torque (rotational force).
---@param torque Vec3
function RigidbodyComponent:AddTorque(torque) end
---Applies instant angular impulse.
---@param impulse Vec3
function RigidbodyComponent:AddAngularImpulse(impulse) end
---Adds linear velocity directly.
---@param velocity Vec3
function RigidbodyComponent:AddLinearVelocity(velocity) end

---@class CharacterBodyComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field position Vec3 Character position
---@field rotation Quat Character rotation
---@field velocity Vec3 Linear velocity
---@field up Vec3 Up direction
---@field maxSlopeAngle number Maximum walkable slope in degrees
---@field isGrounded boolean Whether character is standing on ground
---@field isOnSteepGround boolean Whether character is on too steep ground
---@field isInAir boolean Whether character is airborne
---@field isNotSupported boolean Whether ground cannot support character
---@field centerOfMass Vec3 Center of mass (read-only)
---@field groundNormal Vec3 Normal vector of surface under character (read-only)
---@field groundVelocity Vec3 Velocity of platform under character (read-only)
---@field groundPosition Vec3 Position of ground contact (read-only)

---@class AudioEmitterComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field volume number Playback volume [0..1]
---@field pitch number Playback pitch multiplier
---@field loop boolean Whether sound loops
---@field spatial boolean 3D spatialized audio
---@field attenuationModel AttenuationModel Falloff curve
---@field minDistance number Minimum distance before attenuation begins
---@field maxDistance number Maximum audible distance
---@field rolloff number Distance attenuation rate
---@field dopplerFactor number Doppler effect multiplier
---@field busID integer Audio bus ID
---@field playOnCreate boolean Play automatically on start
local AudioEmitterComponent = {}
function AudioEmitterComponent:Play() end
function AudioEmitterComponent:Stop() end
function AudioEmitterComponent:Pause() end
function AudioEmitterComponent:Resume() end
---@return boolean
function AudioEmitterComponent:IsPlaying() end
---@return boolean
function AudioEmitterComponent:IsPaused() end
---@param seconds number
function AudioEmitterComponent:Seek(seconds) end
---@return number
function AudioEmitterComponent:GetPlaybackPosition() end
---@param handle AssetHandle
function AudioEmitterComponent:SetClip(handle) end

---@class AnimatorComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field isPlaying boolean Whether animation is playing
---@field currentTime number Current time in clip in seconds
---@field speed number Playback speed multiplier
---@field currentClipName string Active clip name
---@field duration number Clip duration in seconds (read-only)
---@field loopMode LoopMode Loop mode (read-only)
local AnimatorComponent = {}
---Plays an animation clip.
---@overload fun(self: AnimatorComponent)
---@overload fun(self: AnimatorComponent, clipName: string)
---@overload fun(self: AnimatorComponent, clipName: string, blendTime: number)
---@param clipName? string
---@param blendTime? number Crossfade blend duration in seconds
function AnimatorComponent:Play(clipName, blendTime) end
function AnimatorComponent:Pause() end
function AnimatorComponent:Stop() end
---@param clipName string
function AnimatorComponent:SetClip(clipName) end
---@param clipName string
---@return boolean
function AnimatorComponent:hasClip(clipName) end

---@class ParticleEmitterComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field emitting boolean
---@field amount integer
---@field lifetime number
---@field oneShot boolean
---@field speedScale number
---@field explosiveness number
---@field randomness number
---@field lifetimeRandomness number
---@field localCoords boolean
---@field emitterShape ParticleEmitterShape
---@field shapeDimensions Vec3
---@field direction Vec3
---@field spread number
---@field gravity Vec3
---@field linearVelocityMin number
---@field linearVelocityMax number
---@field angularVelocityMin number
---@field angularVelocityMax number
---@field rotationMin number
---@field rotationMax number
---@field linearAccelMin number
---@field linearAccelMax number
---@field dampingMin number
---@field dampingMax number
---@field scaleMin number
---@field scaleMax number
---@field scaleEnd number
---@field startColor Vec4
---@field endColor Vec4

---@class RectTransformComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field position Vec2 Anchor-relative position
---@field size Vec2 Dimensions in pixels
---@field anchorMin Vec2 Normalized anchor min [0..1]
---@field anchorMax Vec2 Normalized anchor max [0..1]
---@field pivot Vec2 Normalized pivot [0..1]
---@field rotation number Rotation in degrees
---@field scale Vec2 Scale factor
---@field raycastTarget boolean Whether clickable by mouse

---@class UICanvasComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field sortOrder integer Rendering depth order
---@field billboardMode integer Billboard mode (0=Disabled, 1=Spherical, 2=Cylindrical)
---@field isBillboard boolean Whether canvas faces camera
---@field constantScreenSize boolean Fix pixel size regardless of distance
---@field constantScaleFactor number Scale multiplier for constant screen size

---@class UIImageComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field color Vec4 Tint color (RGBA)

---@class UITextComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field text string Label text
---@field color Vec4 Text color (RGBA)
---@field fontSize number Font size in points
---@field isBold boolean
---@field isOutline boolean
---@field outlineSize number
---@field outlineColor Vec4
---@field wordWrap boolean
---@field lineSpacing number
---@field characterSpacing number

---@class UIButtonComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field interactable boolean Whether button accepts clicks
---@field text string Button label
---@field textColor Vec4
---@field isBold boolean
---@field isOutline boolean
---@field outlineSize number
---@field outlineColor Vec4
---@field normalColor Vec4
---@field hoverColor Vec4
---@field pressedColor Vec4
---@field luaCallback string
local UIButtonComponent = {}
---Registers a click listener callback function.
---@param callback fun()
function UIButtonComponent:OnClick(callback) end

---@class UISliderComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field value number Current slider value
---@field minValue number Minimum value
---@field maxValue number Maximum value
---@field luaCallback string
local UISliderComponent = {}
---Registers a value changed callback.
---@param callback fun(val: number)
function UISliderComponent:OnValueChanged(callback) end

---@class UIProgressBarComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field value number Progress percentage [0..1]
---@field fillColor Vec4 Fill bar color

---@class UICheckboxComponent : ComponentHandle
---@field IsValid fun(self: ComponentHandle): boolean
---@field isChecked boolean
---@field interactable boolean
---@field label string
---@field fontSize number
---@field luaCallback string
local UICheckboxComponent = {}
---Registers a toggle change callback.
---@param callback fun(checked: boolean)
function UICheckboxComponent:OnValueChanged(callback) end


-------------------------------------------------------------------------------
-- ENTITY & SCENE
-------------------------------------------------------------------------------

---@alias ComponentTypeName
---| '"Transform"'
---| '"Camera"'
---| '"DirectionalLight"'
---| '"PointLight"'
---| '"MeshRenderer"'
---| '"Material"'
---| '"Rigidbody"'
---| '"CharacterBody"'
---| '"AudioEmitter"'
---| '"Animator"'
---| '"ParticleEmitter"'
---| '"RectTransform"'
---| '"UICanvas"'
---| '"UIImage"'
---| '"UIText"'
---| '"UIButton"'
---| '"UISlider"'
---| '"UIProgressBar"'
---| '"UICheckbox"'

---@class Entity
---@field name string Entity tag name
---@field active boolean Active status in hierarchy
local Entity = {}

---Checks if entity handle is valid and exists in scene.
---@return boolean
function Entity:IsValid() end

---Gets the entity's tag name.
---@return string
function Entity:GetName() end

---Queues this entity for destruction at end of frame.
function Entity:Destroy() end

---Sets entity active status.
---@param active boolean
function Entity:SetActive(active) end

---Checks if entity is active in hierarchy.
---@return boolean
function Entity:IsActive() end

---Checks if entity itself is active (ignoring parent chain).
---@return boolean
function Entity:IsSelfActive() end

---Gets unique entity UUID.
---@return AssetHandle
function Entity:GetUUID() end

---Sets parent entity. Pass nil to unparent.
---@param parent? Entity
function Entity:SetParent(parent) end

---Gets parent entity or nil if root.
---@return Entity?
function Entity:GetParent() end

---Finds direct or indirect child by tag name.
---@param childName string
---@return Entity?
function Entity:FindChild(childName) end

---Gets attached script class name.
---@return string
function Entity:GetScriptType() end

---Checks if attached script has a method.
---@param methodName string
---@return boolean
function Entity:HasMethod(methodName) end

---Calls a method on the entity's attached Lua script instance.
---@param methodName string
---@param ... any
---@return any
function Entity:CallMethod(methodName, ...) end

---Connects one of this entity's script signals to a method on another entity.
---@param signalName string
---@param target Entity Entity whose script receives the signal
---@param methodName string
---@return integer connectionId Use with DisconnectSignal; 0 means failure
function Entity:ConnectSignal(signalName, target, methodName) end

---Disconnects a signal connection created by this entity.
---@param connectionId integer
---@return boolean disconnected
function Entity:DisconnectSignal(connectionId) end

---Checks if entity has a specific component.
---@param typeName ComponentTypeName|string
---@return boolean
function Entity:HasComponent(typeName) end

---Gets a component attached to this entity.
---@generic T
---@param typeName `T`
---@return T
---@overload fun(self: Entity, typeName: '"Transform"'): TransformComponent
---@overload fun(self: Entity, typeName: '"Camera"'): CameraComponent
---@overload fun(self: Entity, typeName: '"DirectionalLight"'): DirectionalLightComponent
---@overload fun(self: Entity, typeName: '"PointLight"'): PointLightComponent
---@overload fun(self: Entity, typeName: '"MeshRenderer"'): MeshRendererComponent
---@overload fun(self: Entity, typeName: '"Material"'): MaterialComponent
---@overload fun(self: Entity, typeName: '"Rigidbody"'): RigidbodyComponent
---@overload fun(self: Entity, typeName: '"CharacterBody"'): CharacterBodyComponent
---@overload fun(self: Entity, typeName: '"AudioEmitter"'): AudioEmitterComponent
---@overload fun(self: Entity, typeName: '"Animator"'): AnimatorComponent
---@overload fun(self: Entity, typeName: '"ParticleEmitter"'): ParticleEmitterComponent
---@overload fun(self: Entity, typeName: '"RectTransform"'): RectTransformComponent
---@overload fun(self: Entity, typeName: '"UICanvas"'): UICanvasComponent
---@overload fun(self: Entity, typeName: '"UIImage"'): UIImageComponent
---@overload fun(self: Entity, typeName: '"UIText"'): UITextComponent
---@overload fun(self: Entity, typeName: '"UIButton"'): UIButtonComponent
---@overload fun(self: Entity, typeName: '"UISlider"'): UISliderComponent
---@overload fun(self: Entity, typeName: '"UIProgressBar"'): UIProgressBarComponent
---@overload fun(self: Entity, typeName: '"UICheckbox"'): UICheckboxComponent
---@overload fun(self: Entity, typeName: string): any
function Entity:GetComponent(typeName) end

---Adds a component to this entity.
---@generic T
---@param typeName `T`
---@return T
---@overload fun(self: Entity, typeName: '"PointLight"'): PointLightComponent
---@overload fun(self: Entity, typeName: '"DirectionalLight"'): DirectionalLightComponent
---@overload fun(self: Entity, typeName: '"Camera"'): CameraComponent
---@overload fun(self: Entity, typeName: '"Rigidbody"'): RigidbodyComponent
---@overload fun(self: Entity, typeName: '"AudioEmitter"'): AudioEmitterComponent
---@overload fun(self: Entity, typeName: '"Animator"'): AnimatorComponent
---@overload fun(self: Entity, typeName: '"RectTransform"'): RectTransformComponent
---@overload fun(self: Entity, typeName: '"UICanvas"'): UICanvasComponent
---@overload fun(self: Entity, typeName: '"UIImage"'): UIImageComponent
---@overload fun(self: Entity, typeName: '"UIText"'): UITextComponent
---@overload fun(self: Entity, typeName: '"UIButton"'): UIButtonComponent
---@overload fun(self: Entity, typeName: '"UISlider"'): UISliderComponent
---@overload fun(self: Entity, typeName: '"UIProgressBar"'): UIProgressBarComponent
---@overload fun(self: Entity, typeName: '"UICheckbox"'): UICheckboxComponent
---@overload fun(self: Entity, typeName: string): any
function Entity:AddComponent(typeName) end

_G.Entity = Entity


---@class Scene
local Scene = {}

---Creates a new empty entity in the active scene.
---@param name string
---@return Entity
function Scene:CreateEntity(name) end

---Destroys an entity.
---@param entity Entity
function Scene:DestroyEntity(entity) end

---Finds the first entity with the given name in the scene.
---@param name string
---@return Entity?
function Scene:FindEntityByName(name) end

---Instantiates a prefab asset into the scene.
---@param prefab AssetHandle|string AssetHandle or UUID string
---@param position? Vec3 Spawn position
---@param rotation? Quat|Vec3 Spawn rotation
---@param parent? Entity Optional parent entity
---@return Entity
function Scene:Instantiate(prefab, position, rotation, parent) end

_G.Scene = Scene


-------------------------------------------------------------------------------
-- SCRIPT BASE / LIFECYCLE
-------------------------------------------------------------------------------

---@class ScriptInstance
---@field entity Entity The entity owning this script instance
---@field scene Scene The active scene
---@field physics any The physics world
---@field properties table<string, any> Exported properties inspected in editor; use { type = "Entity" } for an entity reference
---@field signals string[] Names of signals declared by this script, e.g. { "Opened" }
---@field [string] any Script-specific fields added by each Lua script
local ScriptInstance = {}

---Waits for a number of seconds. Only use inside a coroutine started with StartCoroutine.
---@param seconds number
function await(seconds) end
_G.await = await

---Starts a coroutine. The callback receives this script instance as its argument.
---@param callback fun(self: ScriptInstance)
---@return integer coroutineId
function ScriptInstance:StartCoroutine(callback) end

---Stops a coroutine started by this script.
---@param coroutineId integer
---@return boolean stopped
function ScriptInstance:StopCoroutine(coroutineId) end

---Runs a callback once after the given number of seconds. The callback receives this script instance.
---@param seconds number
---@param callback fun(self: ScriptInstance)
---@return integer timerId
function ScriptInstance:StartTimer(seconds, callback) end

---Runs a callback repeatedly. The callback receives this script instance.
---@param interval number
---@param callback fun(self: ScriptInstance)
---@return integer timerId
function ScriptInstance:StartRepeatingTimer(interval, callback) end

---Cancels a timer started by this script.
---@param timerId integer
---@return boolean cancelled
function ScriptInstance:CancelTimer(timerId) end

---Emits one of the signals declared in the script's `signals` list.
---@param signalName string
---@param ... any
---@return boolean success
function ScriptInstance:EmitSignal(signalName, ...) end

---Called once when the script is initialized or game starts.
function ScriptInstance:OnCreate() end

---Called after startup scripts have run OnCreate and OnEnabled.
function ScriptInstance:OnReady() end

---Called when the running scene enters the paused state.
function ScriptInstance:OnScenePaused() end

---Called when the running scene resumes from the paused state.
function ScriptInstance:OnSceneResumed() end

---Called every frame.
---@param dt number Delta time in seconds
function ScriptInstance:OnUpdate(dt) end

---Called on every fixed physics step.
---@param fixedDt number Fixed physics step delta time in seconds
function ScriptInstance:OnFixedUpdate(fixedDt) end

---Called after all update passes each frame.
---@param dt number Delta time in seconds
function ScriptInstance:OnLateUpdate(dt) end

---Called when the entity is destroyed or scene stops.
function ScriptInstance:OnDestroy() end

---Called when the script's entity becomes active in the hierarchy.
function ScriptInstance:OnEnabled() end

---Called when the script's entity becomes inactive in the hierarchy.
function ScriptInstance:OnDisabled() end

---Called when physics collision starts.
---@param collision CollisionInfo
function ScriptInstance:OnCollisionEnter(collision) end

---Called while physics collision persists.
---@param collision CollisionInfo
function ScriptInstance:OnCollisionStay(collision) end

---Called when physics collision ends.
---@param collision CollisionInfo
function ScriptInstance:OnCollisionExit(collision) end

---Called when a physics body enters a trigger volume.
---@param other CollisionInfo
function ScriptInstance:OnTriggerEnter(other) end

---Called while the script's entity remains inside a trigger volume.
---@param other CollisionInfo
function ScriptInstance:OnTriggerStay(other) end

---Called when a physics body exits a trigger volume.
---@param other CollisionInfo
function ScriptInstance:OnTriggerExit(other) end

---Called when an animation event marker is triggered.
---@param source Entity
---@param eventName string
---@param parameter string
function ScriptInstance:OnAnimationEvent(source, eventName, parameter) end

---Called when an animation clip starts playing.
---@param clipName string
function ScriptInstance:OnAnimationStarted(clipName) end

---Called when an animation clip finishes playback.
---@param clipName string
function ScriptInstance:OnAnimationFinished(clipName) end

---Called when a looped animation finishes an iteration and restarts.
---@param clipName string
function ScriptInstance:OnAnimationLooped(clipName) end

---Called when audio playback completes on this entity's AudioEmitterComponent.
function ScriptInstance:OnAudioFinished() end

_G.ScriptInstance = ScriptInstance
