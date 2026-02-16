@echo off
REM Primary Launcher
REM This batch file launches the PowerShell version of Primary

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"

REM Launch PowerShell script hidden in background
powershell.exe -WindowStyle Hidden -ExecutionPolicy Bypass -File "%SCRIPT_DIR%Primary.ps1"
