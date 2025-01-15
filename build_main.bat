@echo off
echo --- Building main program ---

g++ -std=c++11 -pthread -o weather_monitor.exe main.cpp -lpthread

if %errorlevel% neq 0 (
    echo --- Compilation failed ---
    exit /b 1
)

echo --- Build successful ---
pause