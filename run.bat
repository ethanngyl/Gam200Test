@echo off
setlocal

:: ---------------------------------------------------------------------------
:: Configuration
:: ---------------------------------------------------------------------------
set GENERATOR=Visual Studio 17 2022
set BUILD_TYPE=Debug
set MODE=game
set ACTION=build
set BUILD_ROOT=out\build

:: ---------------------------------------------------------------------------
:: Parse command line arguments
::   run.bat                          -> game debug
::   run.bat editor                   -> editor debug
::   run.bat game release             -> game release
::   run.bat both                     -> generate+build both game and editor
::   run.bat clean                    -> remove both build trees
::   run.bat editor rebuild           -> clean editor tree then build
:: ---------------------------------------------------------------------------
for %%A in (%*) do (
    if /I "%%~A"=="editor" set MODE=editor
    if /I "%%~A"=="game" set MODE=game
    if /I "%%~A"=="both" set MODE=both
    if /I "%%~A"=="release" set BUILD_TYPE=Release
    if /I "%%~A"=="debug" set BUILD_TYPE=Debug
    if /I "%%~A"=="clean" set ACTION=clean
    if /I "%%~A"=="rebuild" set ACTION=rebuild
)

if /I "%ACTION%"=="clean" goto clean

echo ========================================
echo StructSquad Build
echo   Mode   : %MODE%
echo   Config : %BUILD_TYPE%
echo   Action : %ACTION%
echo ========================================

if /I "%MODE%"=="both" (
    call :build_target game
    if errorlevel 1 exit /b 1
    call :build_target editor
    if errorlevel 1 exit /b 1
    goto end
)

call :build_target %MODE%
if errorlevel 1 exit /b 1
goto end

:build_target
set TARGET_MODE=%~1
set TARGET_BUILD_DIR=%BUILD_ROOT%\vs2022-%TARGET_MODE%-x64-%BUILD_TYPE%

if /I "%TARGET_MODE%"=="editor" (
    set TARGET_CMAKE_ARGS=-DSTRUCTSQUAD_START_IN_EDITOR=ON -DSTRUCTSQUAD_INITIAL_STATE=level3
) else (
    set TARGET_CMAKE_ARGS=-DSTRUCTSQUAD_START_IN_EDITOR=OFF -DSTRUCTSQUAD_INITIAL_STATE=mainmenu
)

if /I "%ACTION%"=="rebuild" (
    echo.
    echo [clean] Removing %TARGET_BUILD_DIR%
    if exist "%TARGET_BUILD_DIR%" rmdir /s /q "%TARGET_BUILD_DIR%"
)

echo.
echo [1/3] Configuring %TARGET_MODE% -> %TARGET_BUILD_DIR%
cmake -S . -B "%TARGET_BUILD_DIR%" -G "%GENERATOR%" -A x64 %TARGET_CMAKE_ARGS%
if errorlevel 1 (
    echo ERROR: CMake configure failed for %TARGET_MODE%!
    exit /b 1
)

echo.
echo [2/3] Building %TARGET_MODE%...
cmake --build "%TARGET_BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
    echo ERROR: Build failed for %TARGET_MODE%!
    exit /b 1
)

echo.
echo [3/3] Complete for %TARGET_MODE%
echo Solution  : %TARGET_BUILD_DIR%\StructSquad.sln
echo Executable: %TARGET_BUILD_DIR%\%BUILD_TYPE%\StructSquad.exe
exit /b 0

:clean
echo Cleaning generated build trees...
if exist "%BUILD_ROOT%\vs2022-editor-x64-Debug" rmdir /s /q "%BUILD_ROOT%\vs2022-editor-x64-Debug"
if exist "%BUILD_ROOT%\vs2022-editor-x64-Release" rmdir /s /q "%BUILD_ROOT%\vs2022-editor-x64-Release"
if exist "%BUILD_ROOT%\vs2022-game-x64-Debug" rmdir /s /q "%BUILD_ROOT%\vs2022-game-x64-Debug"
if exist "%BUILD_ROOT%\vs2022-game-x64-Release" rmdir /s /q "%BUILD_ROOT%\vs2022-game-x64-Release"
echo Done cleaning split solution build trees.
goto end

:end
echo.
echo ========================================
pause