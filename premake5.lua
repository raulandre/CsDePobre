workspace "CsDePobre"
	configurations { "Debug", "Release" }

raylib_src = "raylib/src"

project "Raylib"
	kind "StaticLib"
	language "C"
	targetdir "bin/%{cfg.buildcfg}"

	defines { "_GNU_SOURCE", "PLATFORM_DESKTOP", "GRAPHICS_API_OPENGL_33" }
	buildoptions { "-Wall", "-Wno-missing-braces", "-Werror=pointer-arith", "-fno-strict-aliasing", "-std=c99", "-fPIC", "-O1", "-Werror=implicit-function-declaration" }
	includedirs { raylib_src .. "/external/glfw/include", raylib_src .. "/external/glfw/deps/mingw" }
	files { "raylib/src/*.c", "raylib/src/*.h" }

project "CsDePobre"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}"
	includedirs { raylib_src }
	dependson { "Raylib" }
	
	links { "Raylib", "m", "glfw" }
	buildoptions { "-pthread" }
	files { "*.c", "*.h" }
