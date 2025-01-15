#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

void simulateDevice(const std::string& port) {
#ifdef _WIN32
    HANDLE hPipe = CreateNamedPipe(
        port.c_str(),
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        256,
        256,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка создания канала на Windows! Код ошибки: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Канал успешно создан: " << port << std::endl;

    std::cout << "Ожидание подключения клиента..." << std::endl;
    bool connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (!connected) {
        std::cerr << "Ошибка подключения клиента на Windows! Код ошибки: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return;
    }

    std::cout << "Клиент подключен." << std::endl;
#else
    int fd = open(port.c_str(), O_WRONLY);
    if (fd < 0) {
        std::cerr << "Ошибка открытия порта на macOS/Linux!" << std::endl;
        return;
    }

    std::cout << "Порт успешно открыт: " << port << std::endl;
#endif

    std::srand(std::time(nullptr));

    while (true) {
        float temperature = 20.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 10.0f));
        std::string data = std::to_string(temperature) + "\n";

#ifdef _WIN32
        DWORD bytesWritten;
        bool success = WriteFile(hPipe, data.c_str(), data.size(), &bytesWritten, NULL);
        if (!success) {
            std::cerr << "Ошибка записи в канал на Windows! Код ошибки: " << GetLastError() << std::endl;
            break;
        }
#else
        ssize_t bytesWritten = write(fd, data.c_str(), data.size());
        if (bytesWritten < 0) {
            std::cerr << "Ошибка записи в порт на macOS/Linux!" << std::endl;
            break;
        }
#endif

        std::cout << "Симулятор: отправка температуры: " << temperature << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

#ifdef _WIN32
    CloseHandle(hPipe);
#else
    close(fd);
#endif
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <порт>" << std::endl;
        return 1;
    }

    std::string port = argv[1];
    simulateDevice(port);
    return 0;
}
