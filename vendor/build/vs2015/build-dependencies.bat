@echo off

for %%X in (msbuild.exe) do (set FOUND=%%~$PATH:X)
if not defined FOUND call "%VS140COMNTOOLS%\VsDevCmd.bat"

cd /D %~dp0

msbuild /t:Build /p:Configuration="Debug" /p:Platform="Win32" dependencies.vcxproj
msbuild /t:Build /p:Configuration="Debug" /p:Platform="x64" dependencies.vcxproj
msbuild /t:Build /p:Configuration="Release" /p:Platform="Win32" dependencies.vcxproj
msbuild /t:Build /p:Configuration="Release" /p:Platform="x64" dependencies.vcxproj

pause