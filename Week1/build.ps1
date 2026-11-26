$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot

Write-Host "=== BUILD ESP32 WEEK 1 ===" -ForegroundColor Cyan
Write-Host "Project: $PSScriptRoot"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    Write-Host "Perintah 'pio' belum tersedia di PATH." -ForegroundColor Red
    Write-Host "Buka terminal PlatformIO di VS Code, lalu jalankan: pio run"
    exit 1
}

pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build gagal. Salin error terminal untuk diperiksa." -ForegroundColor Red
    exit $LASTEXITCODE
}

$bin = Join-Path $PSScriptRoot ".pio\build\esp32doit-devkit-v1\firmware.bin"
$elf = Join-Path $PSScriptRoot ".pio\build\esp32doit-devkit-v1\firmware.elf"

if ((Test-Path -LiteralPath $bin) -and (Test-Path -LiteralPath $elf)) {
    Write-Host ""
    Write-Host "BUILD BERHASIL" -ForegroundColor Green
    Write-Host "Firmware BIN: $bin"
    Write-Host "Firmware ELF: $elf"
} else {
    Write-Host "Build selesai tetapi firmware BIN/ELF tidak ditemukan pada path yang diharapkan." -ForegroundColor Red
    exit 2
}
