SOURCE_DIR="../../source/"
VENDOR_DIR="../../vendor/"

solution "Confidant"
configurations { "Debug", "Release" }
language "C++"
includedirs 
{
   SOURCE_DIR .. "cl", 
   SOURCE_DIR .. "lib/include", 
   VENDOR_DIR .. "libsodium/src/libsodium/include", 
   VENDOR_DIR .. "libsodium/src/libsodium/include/sodium", 
   VENDOR_DIR .. "restclient-cpp/include",
   VENDOR_DIR .. "restclient-cpp/vendor/jsoncpp-0.10.5/dist"
}

buildoptions{ "--std=c++11" }

configuration "Debug"
defines { "DEBUG" }
flags { "Symbols" }
targetdir "build/debug"

configuration "Release"
defines { "NDEBUG" }
flags { "OptimizeSpeed",
	"EnableSSE", 
	"EnableSSE2",
	"FloatFast",
	"ExtraWarnings",
	"FatalWarnings",
	"NoFramePointer"}
targetdir "build/release"

-- autogen libsodium
curr_dir = os.getcwd()
os.chdir( VENDOR_DIR .. "libsodium" )
os.execute( "./autogen.sh && ./configure" )
os.chdir( curr_dir )

project "sodium"
kind "StaticLib"
files 
{
   VENDOR_DIR .. "libsodium/src/**.cpp",
   VENDOR_DIR .. "libsodium/src/**.c",
   VENDOR_DIR .. "libsodium/src/**.h"
}

project "restclient"
kind "StaticLib"
files
{
   VENDOR_DIR .. "restclient-cpp/source/**.cpp",
   VENDOR_DIR .. "restclient-cpp/include/**.h",
}

project "json"
kind "StaticLib"
files
{
   VENDOR_DIR .. "restclient-cpp/vendor/jsoncpp-0.10.5/dist/json/**.h",
   VENDOR_DIR .. "restclient-cpp/vendor/jsoncpp-0.10.5/dist/**.cpp",
}

project "Confidant"
files { SOURCE_DIR .. "lib/**.cpp", SOURCE_DIR .. "lib/**.h" }
kind "StaticLib"

project "Client"
files { SOURCE_DIR .. "cl/**.cpp", SOURCE_DIR .. "cl/**.h" }
kind "ConsoleApp"
targetname "Confidant"
links { "Confidant", "sodium", "restclient", "json", "curl"}
