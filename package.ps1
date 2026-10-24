$ErrorActionPreference = 'Stop'
$alarmRoot = $PSScriptRoot
$env:PATH = 'E:\Qt\6.11.1\msvc2022_64\bin;' + $env:PATH
$env:QT_PLUGIN_PATH = ''
$env:QT_QPA_PLATFORM_PLUGIN_PATH = ''
$env:QML_IMPORT_PATH = ''
$env:QML2_IMPORT_PATH = ''
$env:TEMP = Join-Path $alarmRoot 'work'
$env:TMP = $env:TEMP
$alarmExecutable = Join-Path $alarmRoot 'build\Pomodoro.exe'
$alarmDestination = Join-Path $alarmRoot 'dist'
if (-not (Test-Path -LiteralPath $alarmExecutable)) { throw '请先运行 build.cmd。' }
New-Item -ItemType Directory -Path $alarmDestination -Force | Out-Null
Copy-Item -LiteralPath $alarmExecutable -Destination (Join-Path $alarmDestination 'Pomodoro.exe')
& 'E:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe' --qtpaths 'E:\Qt\6.11.1\msvc2022_64\bin\qtpaths.exe' --release --verbose 0 --no-translations --no-compiler-runtime --qmldir (Join-Path $alarmRoot 'qml') (Join-Path $alarmDestination 'Pomodoro.exe')
if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败，退出码 $LASTEXITCODE" }
Copy-Item -LiteralPath (Join-Path $alarmRoot 'README.md') -Destination (Join-Path $alarmDestination '使用说明.md')
$alarmLicenseDestination = Join-Path $alarmDestination 'LICENSES'
New-Item -ItemType Directory -Path $alarmLicenseDestination -Force | Out-Null
foreach ($alarmLicenseName in @('LGPL-3.0-only.txt','LGPL-2.1-or-later.txt','GPL-3.0-only.txt','Qt-GPL-exception-1.0.txt','BSD-3-Clause.txt','MIT.txt','Apache-2.0.txt','Zlib.txt')) {
    Copy-Item -LiteralPath (Join-Path 'E:\Qt\6.11.1\Src\qtbase\LICENSES' $alarmLicenseName) -Destination (Join-Path $alarmLicenseDestination $alarmLicenseName)
}
$alarmSourceHash = (Get-FileHash -LiteralPath $alarmExecutable -Algorithm SHA256).Hash
$alarmReleaseHash = (Get-FileHash -LiteralPath (Join-Path $alarmDestination 'Pomodoro.exe') -Algorithm SHA256).Hash
if ($alarmSourceHash -ne $alarmReleaseHash) { throw '发布程序校验失败，保留旧版备份。' }
foreach ($alarmOldExecutable in Get-ChildItem -LiteralPath $alarmDestination -Filter 'Pomodoro-before-*.exe' -File) {
    Remove-Item -LiteralPath $alarmOldExecutable.FullName
    Write-Output ('已删除旧版程序：' + $alarmOldExecutable.Name)
}

Write-Output '发布程序：E:\alarm\dist\Pomodoro.exe'

