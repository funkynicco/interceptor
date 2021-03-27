solution "Interceptor"
    language        "C++"
    architecture    "x64"
    targetdir       "build/%{cfg.action}/bin/%{prj.group}/%{cfg.longname}"
    objdir          "build/%{cfg.action}/obj/%{prj.group}/%{prj.name}/%{cfg.longname}"
    --characterset    "MBCS"
    cppdialect      "c++17"
    systemversion   "10.0.19041.0"

    pchheader "StdAfx.h"
    pchsource "%{prj.group}/%{prj.name}/src/StdAfx.cpp"

    configurations {
        "Debug",
        "Release",
    }

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "NL_ARCHITECTURE_X64",
    }

    flags {
        "MultiProcessorCompile"
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "on"

    filter "configurations:Release"
        optimize "on"
        staticruntime "on"
        flags {
            "NoIncrementalLink",
            "LinkTimeOptimization",
        }

    filter {}

    group "App"
    project "Interceptor"
        --dependson "TestLib"
        location "%{prj.group}/%{prj.name}"
        kind "ConsoleApp"

        files {
            "%{prj.group}/%{prj.name}/src/**.*",
        }

        includedirs {
            "%{prj.group}/%{prj.name}/src",           
        }

        libdirs {
        }

        links {
            "ws2_32.lib",
        }
    
    group "Libraries"
        include "lib/nativelib/premake5.project.lua"
--    project "TestLib"
--        location "%{prj.group}/%{prj.name}"
--        kind "StaticLib"
--
--        files {
--            "%{prj.group}/%{prj.name}/src/**.*",
--            "%{prj.group}/%{prj.name}/include/**.*",
--        }
--
--        includedirs {
--            "%{prj.group}/%{prj.name}/src",
--        }