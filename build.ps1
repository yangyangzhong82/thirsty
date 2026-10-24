$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot
try {
    foreach ($targetType in @('server', 'client')) {
        & xmake f -y -p windows -a x64 -m release "--target_type=$targetType"
        if ($LASTEXITCODE -ne 0) { throw "配置 $targetType 版本失败" }

        & xmake -y
        if ($LASTEXITCODE -ne 0) { throw "编译 $targetType 版本失败" }

        $packageDir = Join-Path $PSScriptRoot "bin/$targetType/thirsty"
        New-Item -ItemType Directory -Path $packageDir -Force | Out-Null
        foreach ($name in @('manifest.json', 'thirsty.dll', 'thirsty.pdb')) {
            Copy-Item -LiteralPath (Join-Path $PSScriptRoot "bin/thirsty/$name") -Destination $packageDir -Force
        }
        foreach ($kind in @('behavior_packs', 'resource_packs')) {
            $destination = Join-Path $packageDir $kind
            New-Item -ItemType Directory -Path $destination -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path $PSScriptRoot "bin/thirsty/$kind/thirsty") -Destination $destination -Recurse -Force
        }
    }
}
finally {
    Pop-Location
}
