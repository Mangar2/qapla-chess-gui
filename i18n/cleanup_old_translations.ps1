# cleanup_old_translations.ps1
# Removes translation entries that haven't been used for a specified number of days
# Usage: .\cleanup_old_translations.ps1 [-DaysOld 90] [-DryRun]

param(
    [int]$DaysOld = 90,
    [switch]$DryRun = $false
)

$ErrorActionPreference = "Stop"

function Get-TranslationTimestamp {
    param([string]$Line)
    
    # Check if line is in new format: {s:"...",t:"YYYY-MM-DD"} or {h:"...",s:"...",t:"YYYY-MM-DD"}
    if ($Line -match '\{.*t:"(\d{4}-\d{2}-\d{2})".*\}=') {
        return [datetime]::ParseExact($matches[1], "yyyy-MM-dd", $null)
    }
    
    return $null
}

function Should-RemoveLine {
    param(
        [string]$Line,
        [datetime]$CutoffDate
    )
    
    $timestamp = Get-TranslationTimestamp -Line $Line
    
    if ($null -eq $timestamp) {
        # Old format or no timestamp - keep it
        return $false
    }
    
    return $timestamp -lt $CutoffDate
}

function Process-LanguageFile {
    param(
        [string]$FilePath,
        [datetime]$CutoffDate,
        [bool]$DryRun
    )
    
    if (-not (Test-Path $FilePath)) {
        Write-Host "File not found: $FilePath" -ForegroundColor Yellow
        return
    }
    
    $lines = Get-Content -Path $FilePath
    $newLines = @()
    $removedCount = 0
    $inSection = $false
    
    foreach ($line in $lines) {
        # Track if we're in a Translation section
        if ($line -match '^\[Translation\]') {
            $inSection = $true
            $newLines += $line
            continue
        }
        
        if ($line -match '^\[') {
            $inSection = $false
        }
        
        # Check if this line should be removed
        if ($inSection -and $line -match '^(\{.*\}|[^=]+)=') {
            if (Should-RemoveLine -Line $line -CutoffDate $CutoffDate) {
                $removedCount++
                Write-Host "  Would remove: $($line.Substring(0, [Math]::Min(80, $line.Length)))..." -ForegroundColor Gray
                continue  # Skip this line
            }
        }
        
        $newLines += $line
    }
    
    if ($removedCount -gt 0) {
        Write-Host "File: $FilePath" -ForegroundColor Cyan
        Write-Host "  Removed $removedCount old entries (older than $($CutoffDate.ToString('yyyy-MM-dd')))" -ForegroundColor Green
        
        if (-not $DryRun) {
            # Backup original file
            $backupPath = "$FilePath.bak"
            Copy-Item -Path $FilePath -Destination $backupPath -Force
            Write-Host "  Backup created: $backupPath" -ForegroundColor Yellow
            
            # Write cleaned content
            $newLines | Set-Content -Path $FilePath -Encoding UTF8
            Write-Host "  File updated!" -ForegroundColor Green
        } else {
            Write-Host "  [DRY RUN - No changes made]" -ForegroundColor Yellow
        }
    } else {
        Write-Host "File: $FilePath - No old entries found" -ForegroundColor DarkGray
    }
}

# Main script
Write-Host "=== Translation Cleanup Tool ===" -ForegroundColor Cyan
Write-Host "Removing entries older than $DaysOld days" -ForegroundColor Cyan
if ($DryRun) {
    Write-Host "[DRY RUN MODE - No files will be modified]" -ForegroundColor Yellow
}
Write-Host ""

$cutoffDate = (Get-Date).AddDays(-$DaysOld)
Write-Host "Cutoff date: $($cutoffDate.ToString('yyyy-MM-dd'))" -ForegroundColor White
Write-Host ""

# Process all .lang files
$langFiles = Get-ChildItem -Path $PSScriptRoot -Filter "*.lang"

foreach ($file in $langFiles) {
    Process-LanguageFile -FilePath $file.FullName -CutoffDate $cutoffDate -DryRun $DryRun
    Write-Host ""
}

Write-Host "=== Cleanup Complete ===" -ForegroundColor Cyan
if ($DryRun) {
    Write-Host "Run without -DryRun to actually remove old entries" -ForegroundColor Yellow
}
