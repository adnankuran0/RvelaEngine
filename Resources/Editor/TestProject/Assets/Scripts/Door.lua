Door = {
    className = "Door"
}
function Door:OnCreate()
    self.isOpen = false

end

function Door:interact()
    print("interacted with door!")
    self.isOpen = not self.isOpen
    print(self.isOpen)
end


return Door


