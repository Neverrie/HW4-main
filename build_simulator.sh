#!/bin/bash
echo "--- Сборка симулятора устройства ---"

g++ -std=c++11 -o simulator simulator.cpp

if [ $? -ne 0 ]; then
    echo "--- Ошибка компиляции ---"
    exit 1
fi

echo "--- Сборка успешно завершена ---"
