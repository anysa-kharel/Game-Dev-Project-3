@echo off
setlocal
set "CA_PROJECT_DIR=%~dp0"
set "CA_UE_ROOT=%~1"
if "%CA_UE_ROOT%"=="" set "CA_UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
if not exist "%CA_UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 was not found. Pass its install folder as the first argument.
    pause
    exit /b 1
)
call "%CA_UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ShootingEditor Win64 Development "%CA_PROJECT_DIR%Shooting.uproject" -waitmutex -MaxParallelActions=2
if errorlevel 1 (
    echo Build failed. Install the C++ game-development workload for Visual Studio.
    pause
    exit /b 1
)
start "" "%CA_UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%CA_PROJECT_DIR%Shooting.uproject"
endlocal
