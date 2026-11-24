@echo off
setlocal

rem Override FASM before running if it is not available on PATH:
rem   set FASM=C:\path\to\FASM.EXE
if not defined FASM set "FASM=fasm.exe"
set "OUT=build"

if not exist "%OUT%" mkdir "%OUT%"

echo [1/3] Assemble stage1...
"%FASM%" boot\stage1.asm "%OUT%\stage1.bin"
if errorlevel 1 goto :error

echo [2/3] Assemble stage2...
"%FASM%" boot\stage2.asm "%OUT%\stage2.bin"
if errorlevel 1 goto :error

echo [3/3] Assemble kernel...
"%FASM%" kernel\entry.asm "%OUT%\kernel.bin"
if errorlevel 1 goto :error

echo [4/4] Create FAT12 disk image...
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mkfat12.ps1 ^
  -Stage1 "%OUT%\stage1.bin" ^
  -Stage2 "%OUT%\stage2.bin" ^
  -Kernel "%OUT%\kernel.bin" ^
  -Output "%OUT%\bingo.img"
if errorlevel 1 goto :error

echo.
echo Built: %OUT%\bingo.img
goto :eof

:error
echo BUILD FAILED
exit /b 1
