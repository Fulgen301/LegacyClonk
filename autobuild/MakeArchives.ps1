param(
    [Parameter(Mandatory = $true)]
    [string] $OS,

    [Parameter(Mandatory = $true)]
    [string] $Arch,

    [Parameter(Mandatory = $true)]
    [string] $PlatformSuffix,

    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

$tempDir = [System.IO.Directory]::CreateTempSubdirectory()

function CreateArchive([string] $prefix) {
    $targetZip = [System.IO.Path]::GetFullPath("$(Join-Path $OutDir "lc_$($prefix ? "${prefix}_" : '')${Env:VERSION}_$PlatformSuffix").zip", $PWD)
    [System.IO.Compression.ZipFile]::CreateFromDirectory($tempDir, $targetZip)
}

Move-Item -Path 'clonk*' -Destination "$tempDir"
Move-Item -Path 'c4group*' -Destination "$tempDir"
Move-Item -Path @('System.c4g', 'Graphics.c4g') -Destination "$tempDir"

CreateArchive ''

Move-Item -Path *.c4? -Destination "$tempDir"
CreateArchive 'full'
