@echo off
echo Building Mini-Torrent...

REM Check if cl.exe (Visual Studio compiler) is available
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Visual Studio compiler not found in PATH.
    echo Please run this from a Visual Studio Developer Command Prompt
    echo or run vcvarsall.bat first.
    exit /b 1
)

REM Compile common files
echo Compiling common files...
cl.exe /EHsc /std:c++14 /c common.cpp network.cpp fileutils.cpp

REM Compile and link tracker
echo Compiling tracker...
cl.exe /EHsc /std:c++14 tracker.cpp common.obj network.obj fileutils.obj /link ws2_32.lib /out:tracker.exe

REM Compile and link peer
echo Compiling peer...
cl.exe /EHsc /std:c++14 peer.cpp common.obj network.obj fileutils.obj /link ws2_32.lib /out:peer.exe

echo Build completed successfully!