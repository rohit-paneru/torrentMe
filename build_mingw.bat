@echo off
echo Building Mini-Torrent with MinGW...

REM Check if g++ is available
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo MinGW g++ compiler not found in PATH.
    echo Please install MinGW and add it to your PATH.
    exit /b 1
)

REM Compile common files
echo Compiling common files...
g++ -std=c++11 -c common.cpp network.cpp fileutils.cpp

REM Compile and link tracker
echo Compiling tracker...
g++ -std=c++11 -o tracker.exe tracker.cpp common.o network.o fileutils.o -lws2_32

REM Compile and link peer
echo Compiling peer...
g++ -std=c++11 -o peer.exe peer.cpp common.o network.o fileutils.o -lws2_32

echo Build completed successfully!