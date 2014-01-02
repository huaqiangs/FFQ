@echo off

set DIR=%~dp0
set WIN32_PATH=%DIR%proj.win32\Debug.win32\FFQRes\

echo - config:
echo DIR		= %DIR%
echo WIN32_PATH		= %WIN32_PATH%


echo - cleanup
if exist "%WIN32_PATH%res" rmdir /s /q "%WIN32_PATH%res"
if exist "%WIN32_PATH%src" rmdir /s /q "%WIN32_PATH%src"

echo - copy Resources
xcopy /s /q /y "%DIR%Resources\*.*" "%WIN32_PATH%"


echo - TASKKILL /IM FFQ.exe
TASKKILL /IM FFQ.exe

echo CD %DIR%proj.win32\Debug.win32
CD %DIR%proj.win32\Debug.win32

echo "Start FFQ.exe"

start FFQ.exe

REM pause