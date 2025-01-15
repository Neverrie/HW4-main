@echo off
echo --- Building simulator ---

g++ -std=c++11 -o simulator.exe simulator.cpp

if %errorlevel% neq 0 (
    echo --- Compilation failed ---
    exit /b 1
)

echo --- Build successful ---
pause