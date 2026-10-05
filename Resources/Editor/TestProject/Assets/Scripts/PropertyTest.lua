PropertyTest = {}

PropertyTest.properties = {
    testFloat       = 3.14,
    testSliderFloat = { type = "float", default = 5.0, min = 0.0, max = 50.0, step = 0.5 },
    testInt         = 42,
    testSliderInt   = { type = "int", default = 10, min = 0, max = 100 },
    testBool        = true,
    testString      = "Hello Rvela!",
    testVec2        = Vec2.new(1.5, 2.5),
    testVec3        = Vec3.new(0.0, 5.0, -2.0),
    testVec4        = Vec4.new(1.0, 2.0, 3.0, 4.0),
    testColor       = { type = "color", default = Vec4.new(0.2, 0.8, 0.4, 1.0) },
    testPrefab      = { type = "AssetHandle", default = "3c17201c-5ca4-489d-bed4-6df228fefad9" }
}


return PropertyTest
