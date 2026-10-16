---@class SingletonTest : ScriptInstance
local SingletonTest = {}

SingletonTest.className = "SingletonTest"

function SingletonTest:OnReady()
    print("Singleton works!")
end

function SingletonTest:OnUpdate(dt)
end

return SingletonTest
