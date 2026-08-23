<#
.SYNOPSIS
    Runs the GUI integration tests from the repository root.

.EXAMPLE
    test\integration\run.ps1
    test\integration\run.ps1 --filter tournament-*
    test\integration\run.ps1 --config release

.NOTES
    The tests open a real window, which is the point of this suite. A Windows machine without a
    graphics driver -- a bare CI runner, for instance -- offers only OpenGL 1.1, while the GUI
    needs 3.3; there, put a software Mesa opengl32.dll next to the executable, or use a runner
    with a desktop session.
#>

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $repoRoot
try {
    python3 test/integration/test_runner.py @args
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
