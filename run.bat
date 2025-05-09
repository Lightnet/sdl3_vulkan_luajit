@echo off
echo Copying main.lua to build directory...
copy .\main.lua build\main.lua
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy main.lua
    exit /b %ERRORLEVEL%
)

echo Changing to build directory...
cd build
if %ERRORLEVEL% NEQ 0 (
    echo Failed to change directory to build
    exit /b %ERRORLEVEL%
)

echo Running the application...
set VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
hello_world.exe
if %ERRORLEVEL% NEQ 0 (
    echo Application failed to run
    exit /b %ERRORLEVEL%
)

@REM echo Done.