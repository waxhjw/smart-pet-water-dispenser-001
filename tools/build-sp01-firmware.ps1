param(
    [string]$ToolchainBin,
    [string]$MakePath,
    [switch]$Release,
    [switch]$BenchNoPump,
    [switch]$RadarTest,
    [switch]$PumpTest,
    [switch]$BatteryTest,
    [switch]$BatteryPumpTest,
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
    [switch]$RgbTest001,
    [switch]$LowWaterTest001,
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

if ((@($Release, $BenchNoPump, $RadarTest, $PumpTest, $BatteryTest, $BatteryPumpTest, $PumpAdcTest001, $PumpAdcHoldTest002, $PumpAdcPb1Test001, $PumpAdcPa3Test001, $PumpAdcPa6Test001, $FullTest001, $SpecTest001, $SpecTest002, $SpecTest003, $SpecTest004, $SpecTest005, $SpecTest006, $SpecTest007, $SpecTest007Retry30s, $TouchDiagTest001, $LowWaterTest001, $RgbTest001, $RgbIsolatedPa5Test001, $RgbIsolatedPb3Test001, $RgbComboPa5Pa1Test001, $RgbComboScanTest001, $RgbAllOnTest001, $RgbWhiteBalanceTest001, $RgbTunerTest001, $RgbTristateScanTest001, $RgbSingleChannel) | Where-Object { $_ }).Count -gt 1) {
    throw "Build profiles are mutually exclusive."
}
if ($RgbSingleActiveHigh -and -not $RgbSingleChannel) {
    throw "RgbSingleActiveHigh requires RgbSingleChannel."
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$candidateLetters = @("Z", "Y", "X", "W")
$driveLetter = $null

foreach ($candidate in $candidateLetters) {
    if (-not (Get-PSDrive -Name $candidate -ErrorAction SilentlyContinue)) {
        $driveLetter = $candidate
        break
    }
}

if (-not $driveLetter) {
    throw "No free temporary drive letter was found."
}

$mappedDrive = "${driveLetter}:"
$mappedRoot = $mappedDrive
if (-not $MakePath) {
    foreach ($name in @("mingw32-make", "make")) {
        $candidateMake = Get-Command $name -ErrorAction SilentlyContinue
        if ($candidateMake) { $MakePath = $candidateMake.Source; break }
    }
}
if (-not $MakePath -or -not (Test-Path $MakePath)) {
    throw "GNU Make not found. Add mingw32-make to PATH or pass -MakePath."
}

if (-not $ToolchainBin) {
    $localToolchain = Join-Path $projectRoot "tools/toolchains/arm-gnu-toolchain-15.2.rel1/bin"
    $pathCompiler = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
    if (Test-Path (Join-Path $localToolchain "arm-none-eabi-gcc.exe")) {
        $ToolchainBin = $localToolchain
    } elseif ($pathCompiler) {
        $ToolchainBin = Split-Path -Parent $pathCompiler.Source
    } else {
        throw "Arm GNU Toolchain not found. Add it to PATH or pass -ToolchainBin."
    }
}
$ToolchainBin = (Resolve-Path -LiteralPath $ToolchainBin).Path.Replace('\', '/')
if ($ToolchainBin.Contains(' ')) {
    throw "Use an Arm toolchain directory without spaces, for example C:/arm-gnu/bin."
}

& subst $mappedDrive $projectRoot
try {
    $targetDir = Join-Path $mappedRoot "firmware/target/py32f002b"
    $toolchainBin = $ToolchainBin

    $makeArgs = @(
        "-C",
        $targetDir,
        "GCC_PATH=$toolchainBin"
    )
    if ($TouchDiagTest001) {
        $makeArgs += "TOUCH_DIAG_TEST=1"
        $makeArgs += "BUILD_DIR=build-touch-diag-test001"
    } elseif ($Release) {
        $makeArgs += "RELEASE_BUILD=1"
        $makeArgs += "BUILD_DIR=build-release"
    } elseif ($RadarTest) {
        $makeArgs += "RADAR_TEST=1"
        $makeArgs += "BUILD_DIR=build-radar-test"
    } elseif ($PumpTest) {
        $makeArgs += "PUMP_TEST=1"
        $makeArgs += "BUILD_DIR=build-pump-test"
    } elseif ($BatteryTest) {
        $makeArgs += "BATTERY_TEST=1"
        $makeArgs += "BUILD_DIR=build-battery-test"
    } elseif ($BatteryPumpTest) {
        $makeArgs += "BATTERY_PUMP_TEST=1"
        $makeArgs += "BUILD_DIR=build-battery-pump-test"
    } elseif ($PumpAdcHoldTest002) {
        $makeArgs += "PUMP_ADC_HOLD_TEST=1"
        $makeArgs += "PUMP_ADC_CHANNEL=1"
        $makeArgs += "BUILD_DIR=build-pump-adc-hold-test002"
    } elseif ($PumpAdcTest001) {
        $makeArgs += "PUMP_ADC_TEST=1"
        $makeArgs += "PUMP_ADC_CHANNEL=1"
        $makeArgs += "BUILD_DIR=build-pump-adc-test001"
    } elseif ($PumpAdcPb1Test001) {
        $makeArgs += "PUMP_ADC_TEST=1"
        $makeArgs += "PUMP_ADC_CHANNEL=0"
        $makeArgs += "BUILD_DIR=build-pump-adc-pb1-test001"
    } elseif ($PumpAdcPa3Test001) {
        $makeArgs += "PUMP_ADC_TEST=1"
        $makeArgs += "PUMP_ADC_CHANNEL=1"
        $makeArgs += "BUILD_DIR=build-pump-adc-pa3-test001"
    } elseif ($PumpAdcPa6Test001) {
        $makeArgs += "PUMP_ADC_TEST=1"
        $makeArgs += "PUMP_ADC_CHANNEL=3"
        $makeArgs += "BUILD_DIR=build-pump-adc-pa6-test001"
    } elseif ($RgbTest001) {
        $makeArgs += "RGB_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-test001"
    } elseif ($LowWaterTest001) {
        $makeArgs += "LOW_WATER_TEST=1"
        $makeArgs += "BUILD_DIR=build-low-water-test001"
    } elseif ($RgbIsolatedPa5Test001) {
        $makeArgs += "RGB_ISOLATED_PA5_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-isolated-pa5-test001"
    } elseif ($RgbIsolatedPb3Test001) {
        $makeArgs += "RGB_ISOLATED_PB3_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-isolated-pb3-test001"
    } elseif ($RgbComboPa5Pa1Test001) {
        $makeArgs += "RGB_COMBO_PA5_PA1_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-combo-pa5-pa1-test001"
    } elseif ($RgbComboScanTest001) {
        $makeArgs += "RGB_COMBO_SCAN_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-combo-scan-test001"
    } elseif ($RgbAllOnTest001) {
        $makeArgs += "RGB_ALL_ON_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-all-on-test001"
    } elseif ($RgbWhiteBalanceTest001) {
        $makeArgs += "RGB_WHITE_BALANCE_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-white-balance-test001"
    } elseif ($RgbTunerTest001) {
        $makeArgs += "RGB_TUNER_TEST=1"
        $makeArgs += "BUILD_DIR=build-rgb-tuner-test001"
    } elseif ($RgbTristateScanTest001) {
        $makeArgs += "RGB_COMBO_SCAN_TEST=1"
        $makeArgs += "RGB_TRISTATE=1"
        $makeArgs += "BUILD_DIR=build-rgb-tristate-scan-test001"
    } elseif ($RgbSingleChannel) {
        $singleLevelName = if ($RgbSingleActiveHigh) { "high-" } else { "" }
        $singleBuildName = "build-rgb-single-$singleLevelName$($RgbSingleChannel.ToLowerInvariant())-test001"
        $makeArgs += "RGB_SINGLE_TEST=1"
        $makeArgs += "RGB_CHANNEL=$RgbSingleChannel"
        if ($RgbSingleActiveHigh) {
            $makeArgs += "RGB_SINGLE_ACTIVE_HIGH=1"
        }
        $makeArgs += "BUILD_DIR=$singleBuildName"
    } elseif ($FullTest001) {
        $makeArgs += "FULL_TEST=1"
        $makeArgs += "BUILD_DIR=build-full-test001"
    } elseif ($SpecTest001) {
        $makeArgs += "SPEC_TEST=1"
        $makeArgs += "BUILD_DIR=build-spec-test001"
    } elseif ($SpecTest002) {
        $makeArgs += "SPEC_TEST002=1"
        $makeArgs += "BUILD_DIR=build-spec-test002"
    } elseif ($SpecTest003) {
        $makeArgs += "SPEC_TEST003=1"
        $makeArgs += "BUILD_DIR=build-spec-test003"
    } elseif ($SpecTest004) {
        $makeArgs += "SPEC_TEST004=1"
        $makeArgs += "BUILD_DIR=build-spec-test004"
    } elseif ($SpecTest005) {
        $makeArgs += "SPEC_TEST005=1"
        $makeArgs += "BUILD_DIR=build-spec-test005"
    } elseif ($SpecTest006) {
        $makeArgs += "SPEC_TEST006=1"
        $makeArgs += "BUILD_DIR=build-spec-test006"
    } elseif ($SpecTest007Retry30s) {
        $makeArgs += "SPEC_TEST007=1"
        $makeArgs += "BUILD_DIR=build-spec-test007-retry30s"
    } elseif ($SpecTest007) {
        $makeArgs += "SPEC_TEST007=1"
        $makeArgs += "BUILD_DIR=build-spec-test007"
    } elseif ($BenchNoPump) {
        $makeArgs += "BENCH_NO_PUMP=1"
        $makeArgs += "BUILD_DIR=build-bench-no-pump"
    }
    $makeArgs += "all"

    & $makePath @makeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "SP01 target build failed with exit code $LASTEXITCODE"
    }
}
finally {
    & subst $mappedDrive /D | Out-Null
}
