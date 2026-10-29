project "RvelaRuntime"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"
   warnings "Off"

   files { "Source/**.h", "Source/**.cpp", "../Vendor/GLAD/src/**.c", }

   defines
   {
        "_CONSOLE",
       "JPH_DEBUG_RENDERER",
       "JPH_OBJECT_STREAM",
       "JPH_USE_SSE4_1",
       "JPH_USE_SSE4_2"
   }

   includedirs
   {
      "Source",
      "../Vendor/GLFW/include",
      "../Vendor/GLAD/include",
	  "../Engine/Source",
      "../Vendor/entt",
      "../Vendor/spdlog/include",
      "../Vendor",
      "../Vendor/glm",
      "../Vendor/nlohmann",
      "../Vendor/robin_map/include",
      "../Vendor/lua/include",
      "../Vendor/Jolt/include",
      "../Vendor/recastnavigation/Recast/Include",
      "../Vendor/recastnavigation/Detour/Include",
      "../Vendor/recastnavigation/DetourCrowd/Include",
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/lua/lib",
      "../Vendor/Jolt/lib",

   }

   links
   {
      "RvelaEngine",
        "glfw3",
        "lua54",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS","GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8","/MP"  }

   filter "configurations:Debug"
       defines { "DEBUG"}
       links { "JoltDebug" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       kind "WindowedApp"
       entrypoint "mainCRTStartup"
       defines { "RELEASE","NDEBUG"  }
       runtime "Release"
       links { "Jolt" }
       optimize "On"
       symbols "On"
       linkoptions { "/LTCG:incremental", "/NODEFAULTLIB:LIBCMT" }

   filter "configurations:Dist"
       kind "WindowedApp"
       entrypoint "mainCRTStartup"
       defines { "DIST", "NDEBUG" }
       links { "Jolt" }
       runtime "Release"
       optimize "On"
       symbols "Off"

    filter "files:../Vendor/GLAD/src/gl.c"
        enablepch "Off"
