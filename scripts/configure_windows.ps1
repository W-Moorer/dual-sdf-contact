param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [string]$BuildDir = "",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$VcpkgRoot = "",
    [switch]$UseVcpkg
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if (-not $BuildDir) {
    $BuildDir = Join-Path $Root ("build/windows-" + $Config.ToLower())
}

$cmakeArgs = @("-S", $Root, "-B", $BuildDir, "-G", $Generator, "-A", "x64")

if ($UseVcpkg -or $env:VCPKG_ROOT -or (Test-Path (Join-Path $Root "third_party/vcpkg/vcpkg.exe"))) {
    if (-not $VcpkgRoot) {
        if ($env:VCPKG_ROOT) {
            $VcpkgRoot = $env:VCPKG_ROOT
        } else {
            $VcpkgRoot = Join-Path $Root "third_party/vcpkg"
        }
    }

    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot/scripts/buildsystems/vcpkg.cmake"
    $cmakeArgs += "-DVCPKG_TARGET_TRIPLET=x64-windows"
}

& cmake @cmakeArgs
