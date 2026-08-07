# Extract VMaps & MMaps, then shutdown
$startTime = Get-Date
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "AzerothCore VMaps & MMaps Extractor" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# --- VMaps ---
Write-Host ""
Write-Host "[1/2] Extracting VMaps (5~30 min)..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path Buildings, vmaps | Out-Null
Remove-Item Buildings\* -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item vmaps\* -Recurse -Force -ErrorAction SilentlyContinue
.\vmap4_extractor.exe
.\vmap4_assembler.exe Buildings vmaps
Remove-Item Buildings -Recurse -Force
Write-Host "[1/2] VMaps done." -ForegroundColor Green

# --- MMaps ---
Write-Host ""
Write-Host "[2/2] Extracting MMaps (1~4 hours)... Please be patient." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path mmaps | Out-Null
Remove-Item mmaps\* -Recurse -Force -ErrorAction SilentlyContinue
.\mmaps_generator.exe
Write-Host "[2/2] MMaps done." -ForegroundColor Green

# --- Summary ---
$elapsed = (Get-Date) - $startTime
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "All done! Elapsed: $($elapsed.Hours)h $($elapsed.Minutes)m $($elapsed.Seconds)s" -ForegroundColor Green
Write-Host "System will shutdown in 30 seconds. Press Ctrl+C to cancel." -ForegroundColor Red

shutdown /s /t 30
