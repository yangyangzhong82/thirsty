$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

$packRoot = Join-Path $PSScriptRoot 'packs'
$outputDir = Join-Path $PSScriptRoot 'bin'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$addonPath = Join-Path $outputDir 'thirsty-items.mcaddon'
if (Test-Path -LiteralPath $addonPath) {
    Remove-Item -LiteralPath $addonPath -Force
}
$archive = [System.IO.Compression.ZipFile]::Open($addonPath, 'Create')
try {
    foreach ($kind in @('behavior_packs', 'resource_packs')) {
        $sourceDir = Join-Path $packRoot "$kind/thirsty"
        foreach ($file in Get-ChildItem -LiteralPath $sourceDir -File -Recurse) {
            $relativeName = $file.FullName.Substring($sourceDir.Length + 1).Replace('\', '/')
            [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $file.FullName, "${kind}_thirsty/$relativeName"
            ) | Out-Null
        }
    }
}
finally { $archive.Dispose() }
Write-Output "已打包物品附加包：$addonPath"
