param(
    [string]$VcpkgRoot = "",
    [switch]$InstallOptionalDeps
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if (-not $VcpkgRoot) {
    $VcpkgRoot = Join-Path $Root "third_party/vcpkg"
}

if (-not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
}

& (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics

$packages = @("eigen3:x64-windows")
if ($InstallOptionalDeps) {
    $packages += @("fcl:x64-windows", "openvdb:x64-windows")
}

& (Join-Path $VcpkgRoot "vcpkg.exe") install @packages

Write-Host "[bootstrap_windows] vcpkg root: $VcpkgRoot"
