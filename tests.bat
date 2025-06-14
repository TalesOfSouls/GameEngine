@echo off
set "start_time=%TIME%"
cls

set "EXE_NAME=tests"
set "DESTINATION_DIR=..\build\tests"

IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST "%DESTINATION_DIR%" mkdir "%DESTINATION_DIR%"

if not defined DevEnvDir (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - previous bat call failed.
    exit /b 1
)

cd "%DESTINATION_DIR%"
del *.pdb > NUL 2> NUL
del *.idb > NUL 2> NUL
cd ..\..\cOMS

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

REM Compile each .cpp file into an executable
cl ^
    %BUILD_FLAGS% /MT /nologo /Gm- /GR- /EHsc /W4 /wd4201 /wd4706 /wd4324 ^
    /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /std:c++23 ^
    /D WIN32 /D _WINDOWS /D _UNICODE /D UNICODE ^
    /D _CRT_SECURE_NO_WARNINGS ^
    /Fo"%DESTINATION_DIR%/" /Fe"%DESTINATION_DIR%\MainTest.exe" %DEBUG_DATA% ^
    tests\MainTest.cpp ^
    /link /INCREMENTAL:no ^
    /SUBSYSTEM:CONSOLE /MACHINE:X64 ^
    kernel32.lib user32.lib gdi32.lib winmm.lib

REM Check if the compilation was successful
if errorlevel 1 (
    echo Compilation failed for MainTest.cpp
    exit /b 1
)

REM Run the compiled executable
"%DESTINATION_DIR%\MainTest.exe"

REM Calculate runtime
set end_time=%time%
set options="tokens=1-4 delims=:.,"
for /f %options% %%a in ("%start_time%") do set start_h=%%a&set /a start_m=100%%b %% 100&set /a start_s=100%%c %% 100&set /a start_ms=100%%d %% 100
for /f %options% %%a in ("%end_time%") do set end_h=%%a&set /a end_m=100%%b %% 100&set /a end_s=100%%c %% 100&set /a end_ms=100%%d %% 100

set /a hours=%end_h%-%start_h%
set /a mins=%end_m%-%start_m%
set /a secs=%end_s%-%start_s%
set /a ms=%end_ms%-%start_ms%
if %ms% lss 0 set /a secs = %secs% - 1 & set /a ms = 100%ms%
if %secs% lss 0 set /a mins = %mins% - 1 & set /a secs = 60%secs%
if %mins% lss 0 set /a hours = %hours% - 1 & set /a mins = 60%mins%
if %hours% lss 0 set /a hours = 24%hours%
if 1%ms% lss 100 set ms=0%ms%

set /a totalsecs = %hours%*3600 + %mins%*60 + %secs%
echo Test (incl. build) time %hours%:%mins%:%secs%.%ms% (%totalsecs%.%ms%s total)