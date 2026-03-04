PhysicsTest = {}
PhysicsTest.rb = nil
PhysicsTest.timer = 0


function PhysicsTest:OnCreate()
    self.rb = self.entity:GetComponent("RigidbodyComponent")
   
end

function PhysicsTest:OnFixedUpdate(dt)
    self.timer = self.timer + dt
    self.physics:AddForce(self.rb, Vec3.new(0,8000,0)) 
end

return PhysicsTest