@echo off
"C:\Program Files\qemu\qemu-system-i386.exe" ^
    -drive if=ide,index=0,format=raw,file=build\bingo.img ^
    -boot c ^
    -serial stdio ^
    -display gtk
