project "Interceptor"
    dependson   "nativelib"
    location    "%{prj.group}/%{prj.name}"
    kind        "ConsoleApp"

    files {
        "%{prj.group}/%{prj.name}/src/**.*",
    }

    includedirs {
        "%{prj.group}/%{prj.name}/src",
        "lib/nativelib/include",
    }

    links {
        "nativelib"
    }