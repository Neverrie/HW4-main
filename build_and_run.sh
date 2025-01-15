#!/bin/bash
echo "--- Сборка симулятора устройства ---"

g++ -std=c++11 -o simulator simulator.cpp

if [ $? -ne 0 ]; then
    echo "--- Ошибка компиляции симулятора ---"
    exit 1
fi

echo "--- Сборка симулятора успешно завершена ---"

echo "--- Сборка основной программы ---"

g++ -std=c++11 -o weather_monitor main.cpp -lpthread

if [ $? -ne 0 ]; then
    echo "--- Ошибка компиляции основной программы ---"
    exit 1
fi

echo "--- Сборка основной программы успешно завершена ---"

echo "--- Запуск симулятора и основной программы, :D хайп ---"


echo "--- Создание виртуального порта ---"
socat -d -d pty,raw,echo=0 pty,raw,echo=0 > ports.txt 2>&1 &
SOCAT_PID=$!


sleep 2


PORT1=$(cat ports.txt | grep "PTY is" | sed -n 1p | awk '{print $NF}')
PORT2=$(cat ports.txt | grep "PTY is" | sed -n 2p | awk '{print $NF}')

echo "--- Виртуальные порты созданы: $PORT1 и $PORT2 ---"


echo "--- Запуск симулятора на порту $PORT1 ---"
./simulator $PORT1 &


echo "--- Запуск основной программы на порту $PORT2, q для выхода из программы ---"
./weather_monitor $PORT2


kill $SOCAT_PID
rm ports.txt
