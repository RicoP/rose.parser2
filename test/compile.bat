@echo off

setlocal EnableDelayedExpansion

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "install_path="

for /f "usebackq tokens=*" %%i in (`"!vswhere!" -latest -prerelease -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "install_path=%%i"
)

if defined install_path (
    echo Setting up compile environment...
    call "!install_path!\VC\Auxiliary\Build\vcvarsall.bat" x64
) else (
    echo Error: MSVC build tools not found.
    echo Please install Visual Studio Community
    exit /b 1
)

DEL *.exe

CL /Fe:test1 /std:c++17 /Ox /I../include /I../externals/mpc/ ../externals/mpc/mpc.c test1.cpp

DEL *.exp *.lib *.obj 

test1.exe