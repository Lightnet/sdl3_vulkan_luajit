@echo off
setlocal EnableDelayedExpansion

:: Set project root directory (where this script is located)
set "PROJECT_ROOT=%~dp0"
set "BUILD_DIR=%PROJECT_ROOT%build"

:: Check if build directory exists, create it if not
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    if errorlevel 1 (
        echo Error: Failed to create build directory.
        exit /b 1
    )
)

:: Path to Visual Studio 2022's vcvarsall.bat (adjust if your install path differs)
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VS_PATH%" (
    echo Error: vcvarsall.bat not found at %VS_PATH%. Adjust the path in the script.
    exit /b 1
)

:: Call vcvarsall.bat to set up the MSVC environment for x64
echo Setting up Visual Studio 2022 environment...
call "%VS_PATH%" x64
if errorlevel 1 (
    echo Error: Failed to initialize VS2022 environment.
    exit /b 1
)

:: Navigate to the build directory
cd /d "%BUILD_DIR%"
if errorlevel 1 (
    echo Error: Failed to change to build directory %BUILD_DIR%.
    exit /b 1
)

:: Run CMake with NMake Makefiles generator
echo Configuring project with CMake...
cmake -G "NMake Makefiles" ..
if errorlevel 1 (
    echo Error: CMake configuration failed.
    exit /b 1
)

:: Build the project with nmake
echo Building project with nmake...
nmake
if errorlevel 1 (
    echo Error: nmake build failed.
    exit /b 1
)

echo Build completed successfully!
endlocal
exit /b 0