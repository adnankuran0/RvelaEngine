---@class Door : ScriptInstance
---@field doorRoot Entity?
---@field animator AnimatorComponent?
---@field isOpen boolean
Door = {
    className = "Door"
}

function Door:OnCreate()
    self.doorRoot = self.entity:GetParent():GetParent()
    self.animator = self.doorRoot:GetComponent("Animator")
    self.isOpen = false
end

function Door:interact()
    if not self.animator or self.animator.isPlaying then
        return
    end

    self.isOpen = not self.isOpen
    if self.isOpen then
        self.animator:Play("DoorOpen")
    else
        self.animator:Play("DoorClose")
    end

end

return Door
