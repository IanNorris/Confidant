@echo off

for %%X in (msbuild.exe) do (set FOUND=%%~$PATH:X)
if not defined FOUND call "%VS140COMNTOOLS%\VsDevCmd.bat"

cd /D %~dp0/../../curl/winbuild

nmake /f Makefile.vc mode=static VC=14 ENABLE_WINSSL=yes GEN_PDB=yes MACHINE=x86
nmake /f Makefile.vc mode=static VC=14 ENABLE_WINSSL=yes GEN_PDB=yes MACHINE=x86 DEBUG=yes
REM nmake /f Makefile.vc mode=static VC=14 ENABLE_WINSSL=yes GEN_PDB=yes MACHINE=x64

cd /D %~dp0/../../libsodium/builds/msvc/vs2015

msbuild /t:Build /p:Configuration="StaticDebug" /p:Platform="Win32" libsodium.sln
msbuild /t:Build /p:Configuration="StaticRelease" /p:Platform="Win32" libsodium.sln

cd /D %~dp0

msbuild /t:Build /p:Configuration="Debug" /p:Platform="Win32" dependencies.vcxproj
REM msbuild /t:Build /p:Configuration="Debug" /p:Platform="x64" dependencies.vcxproj
msbuild /t:Build /p:Configuration="Release" /p:Platform="Win32" dependencies.vcxproj
REM msbuild /t:Build /p:Configuration="Release" /p:Platform="x64" dependencies.vcxproj

pause