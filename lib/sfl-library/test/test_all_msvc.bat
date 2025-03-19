@echo off

setlocal EnableDelayedExpansion

:: List of commands to execute
set commands[0]=make CXX="cl /std:c++14" SILENT=1 -f makefile.msvc
set commands[1]=make CXX="cl /std:c++17" SILENT=1 -f makefile.msvc
set commands[2]=make CXX="cl /std:c++20" SILENT=1 -f makefile.msvc
set commands[3]=make CXX="cl /std:c++latest" SILENT=1 -f makefile.msvc

:: Loop counter
set /a i=0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:loop

if not defined commands[%i%] goto :end

echo :::: Starting test [!commands[%i%]!]

:: Execute command
!commands[%i%]!

:: Stop everything if command failed
if %errorlevel% NEQ 0 (
    echo :::: ERROR. Test failed [!commands[%i%]!]
    echo :::: ALL STOP.
    exit /b 1
)

echo :::: Finished test [!commands[%i%]!]

set /a i+=1

goto :loop

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:end

echo :::: Finished all tests.
echo :::: THE END.

exit /b 0
