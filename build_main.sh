#!/bin/bash
echo "--- Сборка основной программы ---"

g++ -std=c++11 -o weather_monitor main.cpp -lpthread

if [ $? -ne 0 ]; then
    echo "--- Ошибка компиляции ---"
    exit 1
fi

echo "--- Сборка успешно завершена ---"
