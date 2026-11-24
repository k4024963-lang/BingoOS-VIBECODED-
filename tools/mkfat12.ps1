param(
    [Parameter(Mandatory=$true)][string]$Stage1,
    [Parameter(Mandatory=$true)][string]$Stage2,
    [Parameter(Mandatory=$true)][string]$Kernel,
    [Parameter(Mandatory=$true)][string]$Output
)

$ErrorActionPreference = 'Stop'
$SectorSize = 512
$TotalSectors = 2880
$ReservedSectors = 34
$SectorsPerFat = 9
$FatCount = 2
$RootEntries = 224
$RootDirSectors = [int](($RootEntries * 32 + $SectorSize - 1) / $SectorSize)
$Fat1Lba = $ReservedSectors
$Fat2Lba = $Fat1Lba + $SectorsPerFat
$RootLba = $Fat2Lba + $SectorsPerFat
$DataLba = $RootLba + $RootDirSectors

$s1 = [System.IO.File]::ReadAllBytes($Stage1)
$s2 = [System.IO.File]::ReadAllBytes($Stage2)
$k  = [System.IO.File]::ReadAllBytes($Kernel)

if ($s1.Length -ne $SectorSize) {
    throw "stage1 must be exactly 512 bytes; got $($s1.Length)"
}
if ($s2.Length -ne $SectorSize) {
    throw "stage2 must be exactly 512 bytes; got $($s2.Length)"
}
if ($k.Length -gt (32 * $SectorSize)) {
    throw "kernel is $($k.Length) bytes; reserved loader area allows at most 16384 bytes"
}
if ($DataLba -ne 66 -or $DataLba -ge $TotalSectors) {
    throw "Unexpected FAT12 geometry; refusing to create image"
}

$image = New-Object byte[] ($TotalSectors * $SectorSize)
[Array]::Copy($s1, 0, $image, 0, $s1.Length)
[Array]::Copy($s2, 0, $image, $SectorSize, $s2.Length)
[Array]::Copy($k, 0, $image, 2 * $SectorSize, $k.Length)

# FAT12 media/reserved entries: cluster 0 = 0xFF0, cluster 1 = 0xFFF.
$fat = New-Object byte[] ($SectorsPerFat * $SectorSize)
$fat[0] = 0xF0
$fat[1] = 0xFF
$fat[2] = 0xFF
[Array]::Copy($fat, 0, $image, $Fat1Lba * $SectorSize, $fat.Length)
[Array]::Copy($fat, 0, $image, $Fat2Lba * $SectorSize, $fat.Length)

$outDir = Split-Path -Parent $Output
if ($outDir -and !(Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}
[System.IO.File]::WriteAllBytes($Output, $image)

Write-Host "FAT12 image created: $Output"
Write-Host "Geometry: 2880 sectors x 512 bytes"
Write-Host "Reserved: $ReservedSectors sectors (stage1, stage2, kernel)"
Write-Host "FAT1 LBA: $Fat1Lba; FAT2 LBA: $Fat2Lba"
Write-Host "Root directory LBA: $RootLba ($RootDirSectors sectors)"
Write-Host "Data area LBA: $DataLba"
Write-Host "Kernel size: $($k.Length) / 16384 bytes"
