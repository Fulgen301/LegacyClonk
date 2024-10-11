param(
    [Parameter(Mandatory = $true)]
    [string] $GroupPath,
    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

if (!(Test-Path $GroupPath)) {
    Write-Error "Group file not found: $GroupPath"
    exit 1
}

$groupName = [System.IO.Path]::GetFileName($GroupPath)

$parts = ($Env:LC_GROUPS | ConvertFrom-Json -AsHashtable)[$groupName]["parts"]
$tempDir = [System.IO.Directory]::CreateTempSubdirectory()

$partPath = Join-Path $tempDir.FullName $groupName

$updatePath = $groupName + ".c4u"

if ($OutDir) {
    $updatePath = Join-Path $OutDir $updatePath
}

Remove-Item $updatePath -ErrorAction SilentlyContinue

foreach ($part in $parts) {
    Write-Output "Downloading $part..."
    $uri = "https://github.com/legacyclonk/LegacyClonk/releases/download/$part/$groupName"
    Invoke-WebRequest -Uri $uri -OutFile $partPath
    Write-Output "Adding support for $part..."
    & $Env:C4GROUP "$updatePath" -g "$partPath" "$GroupPath" $Env:OBJVERSION
}

