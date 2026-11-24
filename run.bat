@echo off
"C:\Program Files\qemu\qemu-system-i386.exe" ^
    -drive file=build\bingo.img,format=raw,if=ide,index=0 ^
    -boot c ^
    -serial stdio ^
    -display gtk
