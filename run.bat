@echo off
"C:\Program Files\qemu\qemu-system-i386.exe" ^
    -drive format=raw,file=build\bingo.img ^
    -serial stdio ^
    -display gtk