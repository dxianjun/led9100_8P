@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "CLI=D:\HED\CIU32\CIU32Programmer\bin\CIU32Programmer_CLI.exe"
set "CONN=D:\HED\CIU32\CIU32Programmer\util\CIU32PrgCLI\cmsisdap-CIU32F003.conn"

set "HEX_FILE=%~1"
if not defined HEX_FILE (
    set /p "HEX_FILE=Enter the full HEX file path: "
)

set "HEX_FILE=%HEX_FILE:"=%"

if not defined HEX_FILE (
    echo ERROR: No HEX file was supplied.
    exit /b 1
)

if not exist "%CLI%" (
    echo ERROR: CIU32 programmer CLI was not found:
    echo   %CLI%
    exit /b 1
)

if not exist "%CONN%" (
    echo ERROR: CIU32F003 connection configuration was not found:
    echo   %CONN%
    exit /b 1
)

if not exist "%HEX_FILE%" (
    echo ERROR: HEX file was not found:
    echo   %HEX_FILE%
    exit /b 1
)

for %%I in ("%HEX_FILE%") do set "HEX_EXT=%%~xI"
if /I not "%HEX_EXT%"==".hex" (
    echo ERROR: Firmware file must have a .hex extension:
    echo   %HEX_FILE%
    exit /b 1
)

echo.
echo CIU32F003 automatic programmer
echo   Firmware:   %HEX_FILE%
echo   Programmer: %CLI%
echo   Connection: %CONN%
echo.
echo Reading device information...

call "%CLI%" "--connection=%CONN%" --show
set "SHOW_RC=%ERRORLEVEL%"
if not "%SHOW_RC%"=="0" (
    echo ERROR: Unable to read CIU32F003 device information. Exit code: %SHOW_RC%
    exit /b 1
)

echo.
echo Device detected. Programming HEX file automatically...
call "%CLI%" "--connection=%CONN%" "--download=%HEX_FILE%" --verbose
set "DOWNLOAD_RC=%ERRORLEVEL%"
if not "%DOWNLOAD_RC%"=="0" (
    echo ERROR: Firmware programming failed. Exit code: %DOWNLOAD_RC%
    exit /b 1
)

echo.
echo SUCCESS: Firmware programming completed.
exit /b 0
