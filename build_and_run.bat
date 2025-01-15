@echo off
echo --- Building simulator ---

g++ -std=c++11 -o simulator.exe simulator.cpp

if %errorlevel% neq 0 (
    echo --- Simulator compilation failed ---
    exit /b 1
)

echo --- Simulator built successfully ---

echo --- Building main program ---

g++ -std=c++11 -o weather_monitor.exe main.cpp -lpthread

if %errorlevel% neq 0 (
    echo --- Main program compilation failed ---
    exit /b 1
)

echo --- Main program built successfully ---

echo --- Starting simulator and main program ---

start simulator.exe \\.\pipe\TemperaturePipe
start weather_monitor.exe \\.\pipe\TemperaturePipe

pause