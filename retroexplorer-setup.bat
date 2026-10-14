@echo off
setlocal
set "APPDIR=%ProgramFiles%\Retro Explorer"
if not exist "%APPDIR%" mkdir "%APPDIR%" >nul 2>&1
copy "%~dp0retroexplorer.exe" "%APPDIR%\retroexplorer.exe" >nul
copy "%~dp0README.txt" "%APPDIR%\README.txt" >nul
if exist "%USERPROFILE%\Desktop" (
  powershell -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%USERPROFILE%\Desktop\Retro Explorer.lnk'); $s.TargetPath = '%APPDIR%\retroexplorer.exe'; $s.WorkingDirectory = '%APPDIR%'; $s.Save()"
)
echo Retro Explorer installed to %APPDIR%
echo.
echo Launching browser...
start "" "%APPDIR%\retroexplorer.exe"
exit /b 0
