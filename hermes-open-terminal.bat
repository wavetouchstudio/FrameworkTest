@echo off
REM Open a Hermes-ready PowerShell in the folder this .bat lives in, and
REM launch Hermes in it. Double-click it (or launch it) inside a project
REM folder and it opens a window already sitting in that folder, with
REM Hermes running.
setlocal
REM PowerShell does its own Set-Location below instead of relying on
REM inherited cwd — this .bat can be launched via a nested "start" chain
REM (python -> cmd.exe /c start -> this .bat -> start powershell.exe),
REM and cwd inheritance across that chain is not reliable.
start "Hermes" powershell.exe -NoExit -Command "Set-Location -LiteralPath '%~dp0'; $host.UI.RawUI.WindowTitle='Hermes - %~nx0'; Clear-Host; Write-Host ''; Write-Host ' Starting Hermes in this folder...' -ForegroundColor Green; Write-Host ''; hermes"
endlocal
