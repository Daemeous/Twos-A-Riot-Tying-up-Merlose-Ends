@echo off
rem Double-click this (or drop a .cue/.bin onto it) to make a Merlose copy of your disc image.
setlocal
title Two's A Riot - Merlose patcher
cd /d "%~dp0"
set "PY=%~dp0files\python\python.exe"
if exist "%PY%" goto run
set "PY=py"
where py >nul 2>nul && goto run
set "PY=python"
where python >nul 2>nul && goto run
echo The "files" folder is missing. Extract the whole zip again, then run this from the
echo extracted folder.
goto end
:run
"%PY%" "%~dp0files\patch_disc.py" "%~dp0." "%~1"
:end
echo.
pause
