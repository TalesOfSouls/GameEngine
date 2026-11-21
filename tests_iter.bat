@echo off
cls

REM Everything is relative to the repository root (= directory of this script, independent of its name)
REM The tests also run from there (test data paths like "tests/object/model")
pushd "%~dp0"

set "EXE_NAME=tests"
set "DESTINATION_DIR=..\build\tests"

IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST "%DESTINATION_DIR%" mkdir "%DESTINATION_DIR%"

if not defined DevEnvDir (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")

REM vcvars64.bat may change the working directory
cd /d "%~dp0"

REM Target of the developer command prompt (vcvars64.bat = x64, vcvarsarm64.bat/vcvarsall.bat arm64 = arm64)
set "MACHINE="
if /I "%Platform%"=="x64" set "MACHINE=X64"
if /I "%Platform%"=="arm64" set "MACHINE=ARM64"
REM Fallback: target architecture of VsDevCmd (always set by the developer command prompt)
if not defined MACHINE if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" set "MACHINE=X64"
if not defined MACHINE if /I "%VSCMD_ARG_TGT_ARCH%"=="arm64" set "MACHINE=ARM64"

if not defined MACHINE (
    echo ERROR: Platform "%Platform%" is not x64 or arm64 - previous bat call failed.
    popd
    exit /b 1
)

del "%DESTINATION_DIR%\*.pdb" > NUL 2> NUL
del "%DESTINATION_DIR%\*.idb" > NUL 2> NUL

REM Use /showIncludes for include debugging

set BUILD_TYPE=DEBUG
set BUILD_FLAGS=/Od /Z7 /WX /FC /RTC1 /DDEBUG

set "DEBUG_DATA=/Fd"%DESTINATION_DIR%\%EXE_NAME%.pdb" /Fm"%DESTINATION_DIR%\%EXE_NAME%.map""

REM Parse command-line arguments
if "%1"=="-r" (
    set BUILD_TYPE=RELEASE
    set BUILD_FLAGS=/O2 /D NDEBUG

    set DEBUG_DATA=
)
if "%1"=="-d" (
    set BUILD_TYPE=DEBUG
    set BUILD_FLAGS=/Od /Z7 /WX /FC /RTC1 /DDEBUG

    set "DEBUG_DATA=/Fd"%DESTINATION_DIR%\%EXE_NAME%.pdb" /Fm"%DESTINATION_DIR%\%EXE_NAME%.map""
)

REM Find all .cpp files in the current directory and subdirectories
REM %%~nf = file name without extension (a variable set inside the loop would need delayed expansion)
for /R %%f in (*Test.cpp) do (
    REM Compile each .cpp file into an executable
    cl ^
        %BUILD_FLAGS% /MT /nologo /Gm- /GR- /EHsc /W4 /wd4201 /wd4706 /wd4324 ^
        /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /std:c++20 ^
        /D WIN32 /D _WINDOWS /D _UNICODE /D UNICODE ^
        /D _CRT_SECURE_NO_WARNINGS ^
        /Fo"%DESTINATION_DIR%/" /Fe"%DESTINATION_DIR%\%%~nf.exe" %DEBUG_DATA% ^
        "%%f" ^
        /link /INCREMENTAL:no ^
        /SUBSYSTEM:CONSOLE /MACHINE:%MACHINE% ^
        kernel32.lib user32.lib gdi32.lib winmm.lib

    REM Check if the compilation was successful
    if errorlevel 1 (
        echo Compilation failed for %%f
        popd
        exit /b 1
    )

    REM Run the compiled executable
    "%DESTINATION_DIR%\%%~nf.exe"
)

popd
