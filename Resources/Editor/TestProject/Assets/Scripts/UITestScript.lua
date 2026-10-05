-- UITestScript.lua
---@class UITestScript : ScriptInstance
UITestScript = {
    className = "UITestScript"
}

function UITestScript:OnCreate()
    print("[UI Test] UI Test Script initialized!")
end

function UITestScript:OnButtonClick()
    print("[UI Test] Button clicked via Lua callback!")
end

function UITestScript:OnSliderChanged(val)
    print("[UI Test] Slider value changed via Lua callback: " .. tostring(val))
end

function UITestScript:OnCheckboxChanged(isChecked)
    print("[UI Test] Checkbox state changed via Lua callback: " .. tostring(isChecked))
end

return UITestScript
