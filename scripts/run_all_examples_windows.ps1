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

$env:PYTHONPATH = (Join-Path $Root "python") + [IO.Path]::PathSeparator + $env:PYTHONPATH
$env:PYTHONUTF8 = "1"

$ExecutableDir = Join-Path $BuildDir ("bin/" + $Config)
$Examples = @(
    "ex01_nanovdb_hello",
    "ex02_hppfcl_distance",
    "ex03_dual_sdf_gap",
    "ex04_single_step_contact",
    "ex05_compare_backends",
    "ex06_regression_smoke"
)

foreach ($Example in $Examples) {
    python -m baseline.run_example $Example --build-dir $ExecutableDir --config $Config
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

ctest --test-dir $BuildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

python -m pytest --version *> $null
if ($LASTEXITCODE -eq 0) {
    python -m pytest tests/test_python_smoke.py
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
} else {
    Write-Host "[run_all_examples_windows] pytest not available; skipped Python smoke tests."
}
