@echo off
setlocal

set FASM=C:\Users\lucky\Desktop\axionos\prealpha\build01-test\fasm\FASM.EXE
set OUT=build

if not exist %OUT% mkdir %OUT%

echo [1/3] stage1...
%FASM% boot\stage1.asm %OUT%\stage1.bin
if errorlevel 1 goto :error

echo [2/3] stage2...
%FASM% boot\stage2.asm %OUT%\stage2.bin
if errorlevel 1 goto :error

echo [3/3] kernel (entry + main)...
%FASM% kernel\entry.asm %OUT%\kernel.bin
if errorlevel 1 goto :error

echo [image]...
powershell -NoProfile -Command ^
  "[System.IO.File]::WriteAllBytes('%OUT%\bingo.img', (New-Object byte[] 1474560))"

powershell -NoProfile -Command ^
  "$img = [System.IO.File]::ReadAllBytes('%OUT%\bingo.img');" ^
  "$s1 = [System.IO.File]::ReadAllBytes('%OUT%\stage1.bin');" ^
  "$s2 = [System.IO.File]::ReadAllBytes('%OUT%\stage2.bin');" ^
  "$k  = [System.IO.File]::ReadAllBytes('%OUT%\kernel.bin');" ^
  "[Array]::Copy($s1, 0, $img, 0, $s1.Length);" ^
  "[Array]::Copy($s2, 0, $img, 512, $s2.Length);" ^
  "[Array]::Copy($k,  0, $img, 1024, $k.Length);" ^
  "[System.IO.File]::WriteAllBytes('%OUT%\bingo.img', $img)"

echo Built: %OUT%\bingo.img
goto :eof

:error
echo BUILD FAILED
exit /b 1