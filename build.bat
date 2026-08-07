@echo off
REM IRON ^& INVESTMENT - Demo 1.4 build (MinGW-w64 / gcc)
REM Adjust RAYLIB to point at your static raylib build.
set RAYLIB=C:\raylib\raylib

gcc -std=c11 -O2 -s -Wall -Wextra ^
    -DPLATFORM_DESKTOP_RGFW ^
    -I"%RAYLIB%\src" -Isrc ^
    src\main.c ^
    -o iron_demo.exe ^
    -L"%RAYLIB%\src" -lraylib -lopengl32 -lgdi32 -lwinmm ^
    -mwindows

if errorlevel 1 goto :eof
echo.
for %%F in (iron_demo.exe) do echo Binary size: %%~zF bytes  ^(budget 1474560^)
