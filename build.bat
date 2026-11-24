@echo off
setlocal
rem Override FASM if you installed it in another location.
if not defined FASM set "FASM=C:\\Users\\lucky\\Desktop\\axionos\\prealpha\\build01-test\\fasm\\FASM.EXE"
set "OUT=build"
if not exist "%OUT%" mkdir "%OUT%"

echo [1/4] Assemble stage1...
"%FASM%" boot\stage1.asm "%OUT%\stage1.bin"
if errorlevel 1 goto :error

echo [2/4] Assemble stage2...
"%FASM%" boot\stage2.asm "%OUT%\stage2.bin"
if errorlevel 1 goto :error

echo [3/4] Assemble kernel...
"%FASM%" kernel\entry.asm "%OUT%\kernel.bin"
if errorlevel 1 goto :error

echo [4/4] Create FAT12 image...
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mkfat12.ps1 -Stage1 "%OUT%\stage1.bin" -Stage2 "%OUT%\stage2.bin" -Kernel "%OUT%\kernel.bin" -Output "%OUT%\bingo.img"
if errorlevel 1 goto :error

echo Built: %OUT%\bingo.img
goto :eof
:error
echo BUILD FAILED
exit /b 1
