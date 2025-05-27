@echo off
echo Building Mini-Torrent with GCC...

REM Check if g++ is available
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo G++ compiler not found in PATH.
    echo Please install MinGW or TDM-GCC.
    exit /b 1
)

REM Compile common files
echo Compiling common files...
g++ -std=c++14 -c common.cpp network.cpp fileutils.cpp

REM Compile and link tracker
echo Compiling tracker...
g++ -std=c++14 tracker.cpp common.o network.o fileutils.o -o tracker.exe -lws2_32

REM Compile and link peer
echo Compiling peer...
g++ -std=c++14 peer.cpp common.o network.o fileutils.o -o peer.exe -lws2_32

echo Build completed successfully!