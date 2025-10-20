@echo off
setlocal

:: Configuration
set BUILD_DIR=build
set BUILD_TYPE=Debug
set GENERATOR="Visual Studio 17 2022"

:: Parse command line arguments
if "%1"=="clean" goto clean
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="rebuild" goto rebuild

:build
echo ========================================
echo Building StructSquad - %BUILD_TYPE%
echo ========================================

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure CMake
echo.
echo [1/3] Configuring CMake...
cmake -S . -B "%BUILD_DIR%" -G %GENERATOR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

:: Build the project
echo.
echo [2/3] Building project...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo [3/3] Build complete!
echo Executable: %BUILD_DIR%\%BUILD_TYPE%\StructSquad.exe
goto end

:clean
echo Cleaning build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo Build directory removed.
) else (
    echo Build directory doesn't exist.
)
goto end

:rebuild
echo Rebuilding from scratch...
call :clean
call :build
goto end

:end
echo.
echo ========================================
pause