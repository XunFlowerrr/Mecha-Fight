# ===========================================
# Build Distributable Package Script (Windows)
# ===========================================
# This script creates a portable build of mecha_fight
# that can be distributed and run on any machine.
# It scans source code for used resources and only copies those.
#
# Usage:
#   .\build_distributable.ps1
#
# Output:
#   dist\mecha_fight\  - Complete distributable folder
#

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"
$DistDir = Join-Path $ProjectRoot "dist\mecha_fight"
$SrcDir = Join-Path $ProjectRoot "src\mecha_fight"
$ResourcesDir = Join-Path $ProjectRoot "resources"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Building Mecha Fight - Distributable Package" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# ==========================================
# STEP 1: Scan source code for used resources
# ==========================================
Write-Host ""
Write-Host "[1/5] Scanning source code for used resources..." -ForegroundColor Yellow

# Function to extract resource paths from source files
function Get-UsedResources {
  param([string]$SourceDir)

  $usedResources = @{
    Objects  = [System.Collections.Generic.HashSet[string]]::new()
    Audio    = [System.Collections.Generic.HashSet[string]]::new()
    Fonts    = [System.Collections.Generic.HashSet[string]]::new()
    Images   = [System.Collections.Generic.HashSet[string]]::new()
    Textures = [System.Collections.Generic.HashSet[string]]::new()
    Music    = [System.Collections.Generic.HashSet[string]]::new()
  }

  # Get all source files (.cpp, .h)
  $sourceFiles = Get-ChildItem -Path $SourceDir -Recurse -Include "*.cpp", "*.h" -File

  foreach ($file in $sourceFiles) {
    $content = Get-Content -Path $file.FullName -Raw

    # Match patterns like "resources/objects/...", "resources/audio/...", etc.
    $regexMatches = [regex]::Matches($content, '"resources/([^"]+)"')

    foreach ($match in $regexMatches) {
      $resourcePath = $match.Groups[1].Value

      # Categorize by type
      if ($resourcePath -match "^objects/([^/]+)/") {
        $null = $usedResources.Objects.Add($Matches[1])
      }
      elseif ($resourcePath -match "^audio/music/(.+)$") {
        $null = $usedResources.Music.Add($Matches[1])
      }
      elseif ($resourcePath -match "^audio/(.+)$") {
        $null = $usedResources.Audio.Add($Matches[1])
      }
      elseif ($resourcePath -match "^fonts/(.+)$") {
        $null = $usedResources.Fonts.Add($Matches[1])
      }
      elseif ($resourcePath -match "^images/(.+)$") {
        $null = $usedResources.Images.Add($Matches[1])
      }
      elseif ($resourcePath -match "^textures/(.+)$") {
        # For textures, we need the full subpath
        $texPath = $Matches[1]
        # Get the top-level texture folder
        if ($texPath -match "^([^/]+)/") {
          $null = $usedResources.Textures.Add($Matches[1])
        }
        else {
          $null = $usedResources.Textures.Add($texPath)
        }
      }
    }
  }

  return $usedResources
}

$usedResources = Get-UsedResources -SourceDir $SrcDir

Write-Host "  Found used resources:" -ForegroundColor Gray
Write-Host "    - Objects: $($usedResources.Objects.Count) folders" -ForegroundColor Gray
foreach ($obj in $usedResources.Objects) {
  Write-Host "      * $obj" -ForegroundColor DarkGray
}
Write-Host "    - Audio: $($usedResources.Audio.Count) files" -ForegroundColor Gray
Write-Host "    - Music: $($usedResources.Music.Count) files" -ForegroundColor Gray
Write-Host "    - Fonts: $($usedResources.Fonts.Count) files" -ForegroundColor Gray
Write-Host "    - Images: $($usedResources.Images.Count) files" -ForegroundColor Gray
Write-Host "    - Textures: $($usedResources.Textures.Count) folders" -ForegroundColor Gray

# ==========================================
# STEP 2: Configure CMake
# ==========================================
Write-Host ""
Write-Host "[2/5] Configuring CMake with BUILD_DISTRIBUTABLE=ON..." -ForegroundColor Yellow

# Create build directory
if (-not (Test-Path $BuildDir)) {
  New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
Set-Location $BuildDir

cmake -DBUILD_DISTRIBUTABLE=ON -DCMAKE_BUILD_TYPE=Release ..

if ($LASTEXITCODE -ne 0) {
  Write-Host "CMake configuration failed!" -ForegroundColor Red
  Set-Location $ProjectRoot
  exit 1
}

# ==========================================
# STEP 3: Build
# ==========================================
Write-Host ""
Write-Host "[3/5] Building mecha_fight..." -ForegroundColor Yellow
cmake --build . --config Release --target mecha_fight

if ($LASTEXITCODE -ne 0) {
  Write-Host "Build failed!" -ForegroundColor Red
  Set-Location $ProjectRoot
  exit 1
}

# ==========================================
# STEP 4: Create distribution with only used resources
# ==========================================
Write-Host ""
Write-Host "[4/5] Creating optimized distribution package..." -ForegroundColor Yellow

if (Test-Path $DistDir) {
  Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

# Copy executable
$ExePath = Join-Path $ProjectRoot "bin\mecha_fight\Release\mecha_fight.exe"
if (-not (Test-Path $ExePath)) {
  $ExePath = Join-Path $ProjectRoot "bin\mecha_fight\mecha_fight.exe"
}
if (-not (Test-Path $ExePath)) {
  $ExePath = Join-Path $ProjectRoot "bin\mecha_fight\Debug\mecha_fight.exe"
}

if (Test-Path $ExePath) {
  Copy-Item $ExePath -Destination $DistDir
  Write-Host "  [OK] Copied executable" -ForegroundColor Green
}
else {
  Write-Host "  [WARN] Could not find mecha_fight.exe" -ForegroundColor Red
}

# Copy shaders (maintain directory structure)
Write-Host "  - Copying shaders..."
$ShaderSrcDir = Join-Path $ProjectRoot "src\mecha_fight\shaders"
$ShaderDestDir = Join-Path $DistDir "src\mecha_fight\shaders"
if (Test-Path $ShaderSrcDir) {
  New-Item -ItemType Directory -Path $ShaderDestDir -Force | Out-Null
  $ShaderExtensions = @("*.vs", "*.fs", "*.gs", "*.tcs", "*.tes", "*.cs")
  $shaderCount = 0
  foreach ($ext in $ShaderExtensions) {
    $shaders = Get-ChildItem -Path $ShaderSrcDir -Filter $ext -ErrorAction SilentlyContinue
    foreach ($shader in $shaders) {
      Copy-Item $shader.FullName -Destination $ShaderDestDir
      $shaderCount++
    }
  }
  Write-Host "  [OK] Copied $shaderCount shader files" -ForegroundColor Green
}

# Create resources directory
$DistResourcesDir = Join-Path $DistDir "resources"
New-Item -ItemType Directory -Path $DistResourcesDir -Force | Out-Null

# Copy only used object folders
Write-Host "  - Copying used 3D models..."
$ObjectsDestDir = Join-Path $DistResourcesDir "objects"
New-Item -ItemType Directory -Path $ObjectsDestDir -Force | Out-Null
$objectsCopied = 0
$objectsSize = 0
foreach ($objFolder in $usedResources.Objects) {
  $srcPath = Join-Path $ResourcesDir "objects\$objFolder"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $ObjectsDestDir -Recurse
    $folderSize = (Get-ChildItem -Path $srcPath -Recurse -File | Measure-Object -Property Length -Sum).Sum
    $objectsSize += $folderSize
    $objectsCopied++
  }
}
Write-Host "  [OK] Copied $objectsCopied model folders ($([math]::Round($objectsSize / 1MB, 2)) MB)" -ForegroundColor Green

# Copy only used audio files
Write-Host "  - Copying used audio files..."
$AudioDestDir = Join-Path $DistResourcesDir "audio"
New-Item -ItemType Directory -Path $AudioDestDir -Force | Out-Null
$audioCopied = 0
$audioSize = 0
foreach ($audioFile in $usedResources.Audio) {
  $srcPath = Join-Path $ResourcesDir "audio\$audioFile"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $AudioDestDir
    $audioSize += (Get-Item $srcPath).Length
    $audioCopied++
  }
}
Write-Host "  [OK] Copied $audioCopied audio files ($([math]::Round($audioSize / 1MB, 2)) MB)" -ForegroundColor Green

# Copy only used music files
Write-Host "  - Copying used music files..."
$MusicDestDir = Join-Path $AudioDestDir "music"
New-Item -ItemType Directory -Path $MusicDestDir -Force | Out-Null
$musicCopied = 0
$musicSize = 0
foreach ($musicFile in $usedResources.Music) {
  $srcPath = Join-Path $ResourcesDir "audio\music\$musicFile"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $MusicDestDir
    $musicSize += (Get-Item $srcPath).Length
    $musicCopied++
  }
}
Write-Host "  [OK] Copied $musicCopied music files ($([math]::Round($musicSize / 1MB, 2)) MB)" -ForegroundColor Green

# Copy only used font files
Write-Host "  - Copying used font files..."
$FontsDestDir = Join-Path $DistResourcesDir "fonts"
New-Item -ItemType Directory -Path $FontsDestDir -Force | Out-Null
$fontsCopied = 0
foreach ($fontFile in $usedResources.Fonts) {
  $srcPath = Join-Path $ResourcesDir "fonts\$fontFile"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $FontsDestDir
    $fontsCopied++
  }
}
Write-Host "  [OK] Copied $fontsCopied font files" -ForegroundColor Green

# Copy only used image files
Write-Host "  - Copying used image files..."
$ImagesDestDir = Join-Path $DistResourcesDir "images"
New-Item -ItemType Directory -Path $ImagesDestDir -Force | Out-Null
$imagesCopied = 0
foreach ($imageFile in $usedResources.Images) {
  $srcPath = Join-Path $ResourcesDir "images\$imageFile"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $ImagesDestDir
    $imagesCopied++
  }
}
Write-Host "  [OK] Copied $imagesCopied image files" -ForegroundColor Green

# Copy only used texture folders
Write-Host "  - Copying used texture folders..."
$TexturesDestDir = Join-Path $DistResourcesDir "textures"
New-Item -ItemType Directory -Path $TexturesDestDir -Force | Out-Null
$texturesCopied = 0
$texturesSize = 0
foreach ($texFolder in $usedResources.Textures) {
  $srcPath = Join-Path $ResourcesDir "textures\$texFolder"
  if (Test-Path $srcPath) {
    Copy-Item -Path $srcPath -Destination $TexturesDestDir -Recurse
    $folderSize = (Get-ChildItem -Path $srcPath -Recurse -File | Measure-Object -Property Length -Sum).Sum
    $texturesSize += $folderSize
    $texturesCopied++
  }
}
Write-Host "  [OK] Copied $texturesCopied texture folders ($([math]::Round($texturesSize / 1MB, 2)) MB)" -ForegroundColor Green

# Copy required DLLs
Write-Host "  - Copying DLLs..."
$DllDir = Join-Path $ProjectRoot "dlls"
$dllsCopied = 0
if (Test-Path $DllDir) {
  $dlls = Get-ChildItem -Path $DllDir -Filter "*.dll" -ErrorAction SilentlyContinue
  foreach ($dll in $dlls) {
    Copy-Item $dll.FullName -Destination $DistDir
    $dllsCopied++
  }
}
Write-Host "  [OK] Copied $dllsCopied DLL files" -ForegroundColor Green

# ==========================================
# STEP 5: Summary
# ==========================================
Write-Host ""
Write-Host "[5/5] Calculating size savings..." -ForegroundColor Yellow

# Calculate original resources size
$originalSize = (Get-ChildItem -Path $ResourcesDir -Recurse -File | Measure-Object -Property Length -Sum).Sum
$distSize = (Get-ChildItem -Path $DistDir -Recurse -File | Measure-Object -Property Length -Sum).Sum
$copiedResourcesSize = $objectsSize + $audioSize + $musicSize + $texturesSize
$savedSize = $originalSize - $copiedResourcesSize
$savedPercent = if ($originalSize -gt 0) { [math]::Round(($savedSize / $originalSize) * 100, 1) } else { 0 }

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "Distribution package created successfully!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Location: $DistDir" -ForegroundColor White
Write-Host ""
Write-Host "Size comparison:" -ForegroundColor Cyan
Write-Host "  Original resources: $([math]::Round($originalSize / 1MB, 2)) MB" -ForegroundColor Gray
Write-Host "  Copied resources:   $([math]::Round($copiedResourcesSize / 1MB, 2)) MB" -ForegroundColor Gray
Write-Host "  Total dist size:    $([math]::Round($distSize / 1MB, 2)) MB" -ForegroundColor White
Write-Host "  Space saved:        $([math]::Round($savedSize / 1MB, 2)) MB ($savedPercent%)" -ForegroundColor Green
Write-Host ""
Write-Host "Contents:" -ForegroundColor Cyan
Get-ChildItem $DistDir | Format-Table Name, @{N = 'Size'; E = { if ($_.PSIsContainer) { "<DIR>" }else { "{0:N0} KB" -f ($_.Length / 1KB) } } }, LastWriteTime -AutoSize
Write-Host ""
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  cd `"$DistDir`"; .\mecha_fight.exe" -ForegroundColor White
Write-Host "============================================" -ForegroundColor Green

# Return to original directory
Set-Location $ProjectRoot
