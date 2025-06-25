@echo off
REM HybridCAD Build Script for Windows
REM Usage: build.bat [clean|release|debug]

setlocal enabledelayedexpansion

echo HybridCAD Build Script
echo ======================

REM Default build type
set BUILD_TYPE=Release
set BUILD_DIR=build

REM Parse command line arguments
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Clean complete.
    goto :eof
)

if "%1"=="debug" (
    set BUILD_TYPE=Debug
    set BUILD_DIR=build-debug
)

if "%1"=="release" (
    set BUILD_TYPE=Release
    set BUILD_DIR=build-release
)

if not "%1"=="" if not "%1"=="debug" if not "%1"=="release" (
    echo Usage: %0 [clean^|release^|debug]
    exit /b 1
)

echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo.

REM Check for required tools
echo Checking dependencies...

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake is not installed or not in PATH
    exit /b 1
)

REM Check for Visual Studio Build Tools
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: MSBuild is not installed or not in PATH
    echo Please install Visual Studio Build Tools or Visual Studio
    exit /b 1
)

echo CMake found
echo MSBuild found

REM Check for Qt6 (basic check)
if defined Qt6_DIR (
    echo Qt6_DIR is set to: %Qt6_DIR%
) else (
    echo Warning: Qt6_DIR environment variable not set
    echo CMake will try to find Qt6 automatically
)

echo.

REM Create build directory
echo Creating build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo Configuring with CMake...
cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if %errorlevel% neq 0 (
    echo Error: CMake configuration failed
    exit /b 1
)

REM Build
echo Building HybridCAD...
cmake --build . --config %BUILD_TYPE% --parallel

if %errorlevel% neq 0 (
    echo Error: Build failed
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: %cd%\%BUILD_TYPE%\HybridCAD.exe
echo.
echo To run HybridCAD:
echo   cd %BUILD_DIR% ^&^& %BUILD_TYPE%\HybridCAD.exe
echo.
echo To install:
echo   cd %BUILD_DIR% ^&^& cmake --install . --config %BUILD_TYPE% 