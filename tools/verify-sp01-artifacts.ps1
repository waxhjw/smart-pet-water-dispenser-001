param(
    [switch]$Release,
    [switch]$BenchNoPump,
    [switch]$BatteryTest,
    [switch]$PumpAdcTest001,
    [switch]$PumpAdcHoldTest002,
    [switch]$PumpAdcPb1Test001,
    [switch]$PumpAdcPa3Test001,
    [switch]$PumpAdcPa6Test001,
    [Alias("FullTest")]
    [switch]$FullTest001,
    [switch]$SpecTest001,
    [switch]$SpecTest002,
    [switch]$SpecTest003,
    [switch]$SpecTest004,
    [switch]$SpecTest005,
    [switch]$SpecTest006,
    [switch]$SpecTest007,
    [switch]$SpecTest007Retry30s,
    [switch]$TouchDiagTest001,
    [switch]$LowWaterTest001,
    [switch]$RgbTest001,
    [switch]$RgbIsolatedPa5Test001,
    [switch]$RgbIsolatedPb3Test001,
    [switch]$RgbComboPa5Pa1Test001,
    [switch]$RgbComboScanTest001,
    [switch]$RgbAllOnTest001,
    [switch]$RgbWhiteBalanceTest001,
    [switch]$RgbTunerTest001,
    [switch]$RgbTristateScanTest001,
    [ValidateSet("PB3", "PA5", "PA1")]
    [string]$RgbSingleChannel,
    [switch]$RgbSingleActiveHigh
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ((@($Release, $BenchNoPump, $BatteryTest, $PumpAdcTest001, $PumpAdcHoldTest002, $PumpAdcPb1Test001, $PumpAdcPa3Test001, $PumpAdcPa6Test001, $FullTest001, $SpecTest001, $SpecTest002, $SpecTest003, $SpecTest004, $SpecTest005, $SpecTest006, $SpecTest007, $SpecTest007Retry30s, $TouchDiagTest001, $LowWaterTest001, $RgbTest001, $RgbIsolatedPa5Test001, $RgbIsolatedPb3Test001, $RgbComboPa5Pa1Test001, $RgbComboScanTest001, $RgbAllOnTest001, $RgbWhiteBalanceTest001, $RgbTunerTest001, $RgbTristateScanTest001, $RgbSingleChannel) | Where-Object { $_ }).Count -gt 1) {
    throw "Artifact profiles are mutually exclusive."
}
if ($RgbSingleActiveHigh -and -not $RgbSingleChannel) {
    throw "RgbSingleActiveHigh requires RgbSingleChannel."
}

 $buildName = if ($TouchDiagTest001) { "build-touch-diag-test001" } elseif ($Release) { "build-release" } elseif ($BenchNoPump) { "build-bench-no-pump" } elseif ($BatteryTest) { "build-battery-test" } elseif ($PumpAdcHoldTest002) { "build-pump-adc-hold-test002" } elseif ($PumpAdcPb1Test001) { "build-pump-adc-pb1-test001" } elseif ($PumpAdcPa3Test001) { "build-pump-adc-pa3-test001" } elseif ($PumpAdcPa6Test001) { "build-pump-adc-pa6-test001" } elseif ($PumpAdcTest001) { "build-pump-adc-test001" } elseif ($LowWaterTest001) { "build-low-water-test001" } elseif ($RgbWhiteBalanceTest001) { "build-rgb-white-balance-test001" } elseif ($RgbTunerTest001) { "build-rgb-tuner-test001" } elseif ($RgbTest001) { "build-rgb-test001" } elseif ($RgbIsolatedPa5Test001) { "build-rgb-isolated-pa5-test001" } elseif ($RgbIsolatedPb3Test001) { "build-rgb-isolated-pb3-test001" } elseif ($RgbComboPa5Pa1Test001) { "build-rgb-combo-pa5-pa1-test001" } elseif ($RgbComboScanTest001) { "build-rgb-combo-scan-test001" } elseif ($RgbAllOnTest001) { "build-rgb-all-on-test001" } elseif ($RgbTristateScanTest001) { "build-rgb-tristate-scan-test001" } elseif ($RgbSingleChannel) { $singleLevelName = if ($RgbSingleActiveHigh) { "high-" } else { "" }; "build-rgb-single-$singleLevelName$($RgbSingleChannel.ToLowerInvariant())-test001" } elseif ($FullTest001) { "build-full-test001" } elseif ($SpecTest001) { "build-spec-test001" } elseif ($SpecTest002) { "build-spec-test002" } elseif ($SpecTest003) { "build-spec-test003" } elseif ($SpecTest004) { "build-spec-test004" } elseif ($SpecTest005) { "build-spec-test005" } elseif ($SpecTest006) { "build-spec-test006" } elseif ($SpecTest007Retry30s) { "build-spec-test007-retry30s" } elseif ($SpecTest007) { "build-spec-test007" } else { "build" }
$artifactDir = Join-Path $projectRoot "firmware/target/py32f002b/$buildName"
$artifactBase = Join-Path $artifactDir "sp01_py32f002b"

$required = @(
    "$artifactBase.elf",
    "$artifactBase.hex",
    "$artifactBase.bin"
)

foreach ($path in $required) {
    if (-not (Test-Path $path)) {
        throw "Missing firmware artifact: $path"
    }
}

Write-Host "SP01 artifacts verified: $artifactDir"
Get-Item $required | Select-Object Name, Length, LastWriteTime
$hashOutput = certutil -hashfile "$artifactBase.hex" SHA256
if ($LASTEXITCODE -ne 0) {
    throw "certutil failed to calculate SHA-256 for $artifactBase.hex"
}
Write-Host "HEX SHA-256:"
$hashOutput
