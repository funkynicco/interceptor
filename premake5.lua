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
    
    group "Libraries"
        include "lib/nativelib/premake5.project.lua"

    group "App"
        include "premake5.interceptor.lua"