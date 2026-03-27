param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if (-not $BuildDir) {
    $BuildDir = Join-Path $Root ("build/windows-" + $Config.ToLower())
}

& cmake --build $BuildDir --config $Config --parallel
