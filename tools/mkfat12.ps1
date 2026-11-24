param(
 [Parameter(Mandatory=$true)][string]$Stage1,
 [Parameter(Mandatory=$true)][string]$Stage2,
 [Parameter(Mandatory=$true)][string]$Kernel,
 [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$ss=512; $total=2880; $reserved=66; $spf=9; $rootEntries=224
$rootSectors=[int](($rootEntries*32+$ss-1)/$ss)
$fat1=$reserved; $fat2=$fat1+$spf; $rootLba=$fat2+$spf; $dataLba=$rootLba+$rootSectors
$s1=[IO.File]::ReadAllBytes($Stage1); $s2=[IO.File]::ReadAllBytes($Stage2); $k=[IO.File]::ReadAllBytes($Kernel)
$sample=[Text.Encoding]::ASCII.GetBytes("BingoOS FAT12 is online.`r`nThis file is stored on disk.`r`n")
if($s1.Length -ne 512){throw "stage1 must be exactly 512 bytes"}
if($s2.Length -ne 512){throw "stage2 must be exactly 512 bytes"}
if($k.Length -gt (64*512)){throw "kernel exceeds reserved 64-sector loader area"}
if($dataLba -ne 98){throw "Unexpected FAT12 geometry"}
$img=New-Object byte[] ($total*$ss)
[Array]::Copy($s1,0,$img,0,$s1.Length)
[Array]::Copy($s2,0,$img,512,$s2.Length)
[Array]::Copy($k,0,$img,1024,$k.Length)
$fat=New-Object byte[] ($spf*$ss)
$fat[0]=0xF0; $fat[1]=0xFF; $fat[2]=0xFF
# FAT12 cluster 2 = 0xFFF, packed at offset 3.
$fat[3]=0xFF; $fat[4]=0x0F
[Array]::Copy($fat,0,$img,$fat1*$ss,$fat.Length)
[Array]::Copy($fat,0,$img,$fat2*$ss,$fat.Length)
$rootOff=$rootLba*$ss
$n=[Text.Encoding]::ASCII.GetBytes("README  TXT")
[Array]::Copy($n,0,$img,$rootOff,11)
$img[$rootOff+11]=0x20
[BitConverter]::GetBytes([UInt16]2).CopyTo($img,$rootOff+26)
[BitConverter]::GetBytes([UInt32]$sample.Length).CopyTo($img,$rootOff+28)
[Array]::Copy($sample,0,$img,$dataLba*$ss,$sample.Length)
$outDir=Split-Path -Parent $Output
if($outDir -and !(Test-Path $outDir)){New-Item -ItemType Directory -Path $outDir -Force | Out-Null}
[IO.File]::WriteAllBytes($Output,$img)
Write-Host "FAT12 image: $Output"
Write-Host "Reserved=$reserved FAT1=$fat1 FAT2=$fat2 ROOT=$rootLba DATA=$dataLba"
Write-Host "README.TXT bytes=$($sample.Length); kernel bytes=$($k.Length)"
