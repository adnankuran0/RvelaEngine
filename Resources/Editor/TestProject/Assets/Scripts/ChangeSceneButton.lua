---@class ChangeSceneButton : ScriptInstance
local ChangeSceneButton = {}

ChangeSceneButton.className = "ChangeSceneButton"

function ChangeSceneButton:Pressed()
    SceneManager.ChangeSceneAsync("Scenes/TPS.rscene")
end

return ChangeSceneButton
