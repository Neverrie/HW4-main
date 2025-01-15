#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <windows.h>

void simulateDevice(const std::string& pipeName) {
    HANDLE hPipe = CreateNamedPipe(
        pipeName.c_str(),
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        256,
        256,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка создания канала! Код ошибки: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Канал успешно создан: " << pipeName << std::endl;

    std::cout << "Ожидание подключения клиента..." << std::endl;
    bool connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (!connected) {
        std::cerr << "Ошибка подключения клиента! Код ошибки: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return;
    }

    std::cout << "Клиент подключен." << std::endl;

    std::srand(std::time(nullptr));

    while (true) {
        float temperature = 20.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 10.0f));
        std::string data = std::to_string(temperature) + "\n";

        DWORD bytesWritten;
        bool success = WriteFile(
            hPipe,
            data.c_str(),
            data.size(),
            &bytesWritten,
            NULL
        );

        if (!success) {
            std::cerr << "Ошибка записи в канал! Код ошибки: " << GetLastError() << std::endl;
            break;
        }

        std::cout << "Симулятор: отправка температуры: " << temperature << std::endl;

        Sleep(1000);
    }

    CloseHandle(hPipe);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <имя канала>" << std::endl;
        return 1;
    }

    std::string pipeName = argv[1];
    simulateDevice(pipeName);
    return 0;
}