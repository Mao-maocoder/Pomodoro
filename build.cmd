@echo off
setlocal
cd /d "%~dp0"
call "F:\Visual Studio 2022\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
set "PATH=E:\Qt\6.11.1\msvc2022_64\bin;%PATH%"
set "QT_PLUGIN_PATH="
set "QT_QPA_PLATFORM_PLUGIN_PATH="
set "QML_IMPORT_PATH="
set "QML2_IMPORT_PATH="
set "TEMP=%~dp0work"
set "TMP=%~dp0work"
"D:\mingw\mingw64\bin\cmake.exe" -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_MAKE_PROGRAM="D:/mingw/mingw64/bin/ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="E:/Qt/6.11.1/msvc2022_64" -DCMAKE_CXX_COMPILER=cl -DBUILD_TESTING=ON
if errorlevel 1 exit /b %errorlevel%
"D:\mingw\mingw64\bin\cmake.exe" --build "%~dp0build" --parallel 4
if errorlevel 1 exit /b %errorlevel%
endlocal
