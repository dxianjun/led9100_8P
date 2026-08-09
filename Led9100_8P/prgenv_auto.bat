@echo off
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul
echo Setting up environment for CIU32Programmer_CLI usage...

set "WORK_DIR=%~1"
if not defined WORK_DIR set "WORK_DIR=%~dp0"

cd /d "%WORK_DIR%"
if errorlevel 1 (
    echo ERROR: Unable to enter working directory:
    echo   %WORK_DIR%
    exit /b 1
)

set "HEX_FILE=%~2"
if not defined HEX_FILE set "HEX_FILE=.\LED9100ciu.hex"

cmd.exe /K .\program_ciu32f003_auto.bat "%HEX_FILE%"

@echo on
