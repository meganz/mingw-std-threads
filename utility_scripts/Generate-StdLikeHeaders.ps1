
<#
.SYNOPSIS
    Generate std-like headers which you can use just like standard c++'s ones.
    For example include <thread>.
.PARAMETER GccPath
    Path to GCC. Will try to use the default one from $env:Path if not 
    specified.
.PARAMETER MinGWStdThreadsPath
    Path to mingw-std-threads folder. Will try to use $PSScriptRoot/.. if not 
    specified.
.PARAMETER DestinationFolder
    Destination folder where generated headers will be saved to
.PARAMETER GenerateCompilerWrapperWithFileName
    If specified, will be generated a wrapper batch script for g++ which automatically
    adds $DestinationFolder as an include path
.PARAMETER Interactive
    Use this switch if you want to pass parameters interactively
#>
[CmdletBinding(PositionalBinding = $false)]
param (
    # Path of GCC
    [Parameter(Mandatory = $false,
        ValueFromPipelineByPropertyName = $true,
        ParameterSetName = "NonInteractive",
        HelpMessage = "Pathtof GCC. Will try to use the default one from `$env:Path if not specified.")]
    [string]
    $GccPath,
    
    # Path of mingw-std-threads
    [Parameter(Mandatory = $false,
        ValueFromPipelineByPropertyName = $true,
        ParameterSetName = "NonInteractive",
        HelpMessage = "Path to mingw-std-threads folder. Will try to use `$PSScriptRoot/.. if not specified.")]
    [string]
    $MinGWStdThreadsPath,

    # Destination folder path
    [Parameter(Mandatory = $true,
        ValueFromPipelineByPropertyName = $true,
        ParameterSetName = "NonInteractive",
        HelpMessage = "Destination folder where generated headers will be saved to")]
    [ValidateNotNullOrEmpty()]
    [string]
    $DestinationFolder,

    # Compiler wrapper path
    [Parameter(Mandatory = $false,
        ValueFromPipelineByPropertyName = $true,
        ParameterSetName = "NonInteractive",
        HelpMessage = "If specified, will generate a wrapper batch script for g++ which automatically adds `$DestinationFolder as an include path")]
    [string]
    $GenerateCompilerWrapperWithFileName,

    # Interactive Switch
    [Parameter(ParameterSetName = "Interactive")]
    [switch]
    $Interactive = $false
)

# Stop execution when encountering any error (includeing Write-Error command)
$ErrorActionPreference = "Stop";

# headers to be generated
$headers = @("condition_variable", "future", "mutex", "shared_mutex", "thread")

# ask for user input in interactive mode
if ($Interactive) {
    Write-Host "Generate std-like headers which you can use just like standard c++'s ones."
    Write-Host "Something like `"include <thread>`"."
    
    $DestinationFolder = Read-Host -Prompt "Destination folder into which headers will be generated"
    $GccPath = Read-Host -Prompt "Path to GCC, optional. Press Enter to let it be retrieved from PATH"
    $MinGWStdThreadsPath = Read-Host -Prompt "Path to mingw-std-threads folder, optional. Press Enter to use default value"
    $GenerateCompilerWrapperWithFileName = Read-Host "Optional path to which a wrapper batch script for g++ will be created. It will automatically use $DestinationFolder as an include path. Press Enter to skip"
}

if (-not $GccPath) {
    $GccPath = "gcc"
}

# set default value of $MinGWStdThreadsPath
if (-not $MinGWStdThreadsPath) {
    $scriptFilePath = $null
    if ($MyInvocation.MyCommand.CommandType -eq "ExternalScript") {
        $scriptFilePath = $MyInvocation.MyCommand.Definition
    }
    else {
        $scriptFilePath = [Environment]::GetCommandLineArgs()[0]
    }
    $MinGWStdThreadsPath = (Get-Item -LiteralPath $scriptFilePath).Directory.Parent.FullName
}

# Normalize paths
$GccPath = (Get-Command -Name $GccPath).Source
$MinGWStdThreadsPath = Resolve-Path -LiteralPath $MinGWStdThreadsPath
$DestinationFolder = New-Item -Path $DestinationFolder -ItemType "Directory" -Force

Write-Output "GccPath: $GccPath"
Write-Output "MinGWStdThreadsPath: $MinGWStdThreadsPath"
Write-Output "DestinationFolder: $DestinationFolder"
if ($GenerateCompilerWrapperWithFileName) {
    Write-Output "GenerateCompilerWrapperWithFileName: $GenerateCompilerWrapperWithFileName"
}

# Find path of real headers
Write-Output "Retrieving system header search paths..."

$readingIncludePath = $false
# Empty array which will later store include paths
$includePaths = @()

# Launch GCC
$processStartInfo = New-Object -TypeName "System.Diagnostics.ProcessStartInfo"
$processStartInfo.FileName = $GccPath
$processStartInfo.Arguments = "-xc++ -E -v -"
$processStartInfo.RedirectStandardInput = $true
$processStartInfo.RedirectStandardOutput = $true
$processStartInfo.RedirectStandardError = $true
$processStartInfo.UseShellExecute = $false

$outputLines = @()
$gcc = New-Object -TypeName "System.Diagnostics.Process"
try {
    $gcc.StartInfo = $processStartInfo
    $gcc.Start() | Out-Null
    $gcc.StandardInput.Close()
    $gcc.WaitForExit()
    $output = $gcc.StandardError.ReadToEnd()
    $outputLines = $output -split "[\r\n]" | 
        ForEach-Object { return $_.Trim() } |
        Where-Object { return $_.Length -gt 0 }
}
finally {
    $gcc.StandardInput.Dispose()
    $gcc.StandardOutput.Dispose()
    $gcc.StandardError.Dispose()
    $gcc.Dispose()
}

# Parse Output
foreach ($line in $outputLines) {
    if (-not $readingIncludePath) {
        if ($line -match "#include <...> search starts here:") {
            $readingIncludePath = $true
        }
        continue
    }

    if ($line -match "End of search list.") {
        break
    }

    Write-Output "Retrieved search path: $line"
    $includePaths += $line
}

if ($includePaths.Count -eq 0) {
    Write-Error "Error: didn't find any #inlcude <...> search paths"
}

# look for std header paths
Write-Output "Searching for standard headers..."
$stdHeaders = @()
# set a label called "nextHeader" to allow continue with outer loop
:nextHeader foreach ($header in $headers) {
    # check if mingw-std-threads headers exist
    $myHeader = "mingw.$header.h"
    $myHeader = Join-Path -Path $MinGWStdThreadsPath -ChildPath $myHeader
    if (-not (Test-Path -LiteralPath $myHeader -PathType "Leaf")) {
        Write-Error "Error: mingw-std-threads header not found: $myHeader"
    }

    foreach ($inludePath in $includePaths) {
        $fullPath = Join-Path -Path $inludePath -ChildPath $header
        if (Test-Path -LiteralPath $fullPath -PathType "Leaf") {
            $fullPath = (Get-Item -LiteralPath $fullPath).FullName
            $stdHeaders += $fullPath
            Write-Output "Found std header: $fullPath"
            # if found matching header, continue with outer loop
            continue nextHeader
        }
    }

    Write-Error "Error: didn't find $header in any search paths"
}

# generate headers
Write-Output "Generating headers..."
foreach ($stdHeader in $stdHeaders) {
    $headerFileName = (Get-Item -LiteralPath $stdHeader).Name
    $myHeader = "mingw.$headerFileName.h"
    $myHeader = Join-Path -Path $MinGWStdThreadsPath -ChildPath $myHeader
    Write-Output "Generating <$headerFileName> from $myHeader and $stdHeader..."

    # both two headers should already have include guards
    # but we still add a #pragma once just to be safe
    $content = "#pragma once`r`n"
    $content += "#include `"$stdHeader`"`r`n"
    $content += "#include `"$myHeader`"`r`n";

    $outputFileName = Join-Path -Path $DestinationFolder -ChildPath $headerFileName
    Write-Output "Writing file: $outputFileName"

    # use .NET's method to output lines to avoid UTF-8 BOM
    $noBomEncoding = New-Object -TypeName "System.Text.UTF8Encoding" -ArgumentList $false
    [IO.File]::WriteAllText($outputFileName, $content, $noBomEncoding)
}

$message = "Successfully generated std-like headers. Use them by adding "
$message += "`"-I$DestinationFolder`" to your compiler command line parameters"
Write-Output $message

if ($GenerateCompilerWrapperWithFileName) {
    $compilerFolder = Split-Path -LiteralPath $GccPath
    $compiler = Join-Path -Path $compilerFolder -ChildPath "g++"
    $command = "@echo off`r`n"
    $command += "$compiler %* `"-I$DestinationFolder`""
    $wrapper = New-Item -Path $GenerateCompilerWrapperWithFileName -ItemType "File" -Force

    # use .NET's method to output lines to avoid UTF-8 BOM
    $noBomEncoding = New-Object -TypeName "System.Text.UTF8Encoding" -ArgumentList $false
    [IO.File]::WriteAllText($wrapper, $command, $noBomEncoding)

    Write-Output "Wrapper batch script successfully generated to $wrapper"
}