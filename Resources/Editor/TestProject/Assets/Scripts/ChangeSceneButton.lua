---@class ChangeSceneButton : ScriptInstance
local ChangeSceneButton = {}

ChangeSceneButton.className = "ChangeSceneButton"

function ChangeSceneButton:Pressed()
    SceneManager.ChangeScene("Scenes/TPS.rscene")
end

return ChangeSceneButton
