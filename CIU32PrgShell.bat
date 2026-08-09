@echo off
setlocal
set "LAUNCH_DIR=%CD%"
set "FIRMWARE_ARG=%~1"
powershell.exe -NoProfile -Command "$dir=$env:LAUNCH_DIR; $escaped=$dir.Replace('''',''''''); $command='Set-Location -LiteralPath '''+$escaped+''''; $firmware=$env:FIRMWARE_ARG; if(-not $firmware){$archive=([string][char]0x70E7)+([char]0x5F55)+([char]0x6863); $firmware='.\'+$archive+'\20240714\LED9100ciu_260714.hex'}; $firmware=$firmware.Replace('''',''''''); $command+='; Start-Sleep -Seconds 1; & ''.\program_ciu32f003.bat'' '''+$firmware+''''; $encoded=[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($command)); Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList '-NoExit','-EncodedCommand',$encoded"
exit /b %ERRORLEVEL%
