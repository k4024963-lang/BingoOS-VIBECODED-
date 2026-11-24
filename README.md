# BingoOS

BingoOS is a 32-bit x86 hobby operating system written in FASM assembly.

## Current target

- Version: prealpha build01-1.1
- Boot: BIOS, two stages, BIOS EDD LBA reads
- Kernel: 32-bit protected mode
- Console: VGA text mode
- Disk: 1.44 MiB FAT12 image on primary IDE master in QEMU

## Build

Install FASM and make `fasm.exe` available in PATH, or set the `FASM` environment variable to its full executable path.

Run:

```bat
build.bat
run.bat
```

The build creates `build/bingo.img`. The image builder checks that stage1 and stage2 are exactly 512 bytes and that the kernel fits in the 64-sector reserved loader area (32 KiB).

## FAT12 layout

The boot sector contains a FAT12 BPB. Sectors 0-65 are reserved for stage1, stage2, and the kernel. The two FAT copies, root directory, and data region begin at LBA 66.

The image builder seeds a sample `README.TXT` in the root directory.

## FAT12 shell commands

- `ls` — list root directory entries
- `cat README.TXT` — print a file from the root directory

The first driver iteration is deliberately read-only and supports 512-byte sectors, one-sector clusters, the root directory, and 8.3 filenames. Subdirectories, long filenames, file creation, deletion, and FAT write support are not implemented yet.

## Notes

The protected-mode disk driver uses ATA PIO LBA28 on the primary IDE master. BIOS INT 13h is used only by the real-mode boot stages.
