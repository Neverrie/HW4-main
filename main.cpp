#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <windows.h>

std::mutex logMutex;
bool running = true;

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void logTemperature(const std::string& message, const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        std::cout << "Запись в лог: " << message << std::endl; // Отладочный вывод
        logFile << message << std::endl;
    } else {
        std::cerr << "Ошибка открытия файла: " << filename << std::endl;
    }
}

void calculateHourlyAverage(const std::vector<float>& temperatures, const std::string& filename) {
    float sum = 0;
    for (float temp : temperatures) {
        sum += temp;
    }
    float average = sum / temperatures.size();
    logTemperature(getCurrentTime() + " - Средняя температура за час: " + std::to_string(average), filename);
}

void calculateDailyAverage(const std::vector<float>& temperatures, const std::string& filename) {
    float sum = 0;
    for (float temp : temperatures) {
        sum += temp;
    }
    float average = sum / temperatures.size();
    logTemperature(getCurrentTime() + " - Средняя температура за день: " + std::to_string(average), filename);
}

void cleanOldLogs(const std::string& filename, int maxAgeSeconds) {
    std::ifstream logFile(filename);
    std::vector<std::string> lines;
    std::string line;

    auto now = std::chrono::system_clock::now();

    while (std::getline(logFile, line)) {
        std::string timeStr = line.substr(0, 19);
        std::tm tm = {};
        std::istringstream ss(timeStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto logTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        if (std::chrono::duration_cast<std::chrono::seconds>(now - logTime).count() <= maxAgeSeconds) {
            lines.push_back(line);
        }
    }

    std::ofstream outFile(filename);
    for (const auto& l : lines) {
        outFile << l << std::endl;
    }
}

void clearLogFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Ошибка очистки файла: " << filename << std::endl;
    } else {
        std::cout << "Файл очищен: " << filename << std::endl;
    }
}

void inputListener() {
    char input;
    while (running) {
        std::cin >> input;
        if (input == 'q') {
            running = false;
            std::cout << "Завершение работы программы..." << std::endl;
        }
        std::cout << "InputListener: получен ввод: " << input << std::endl;
    }
}

void readTemperature(const std::string& pipeName) {
    HANDLE hPipe = CreateFile(
        pipeName.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия канала! Код ошибки: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Канал успешно открыт: " << pipeName << std::endl;

    std::vector<float> hourlyTemperatures;
    std::vector<float> dailyTemperatures;
    auto lastHour = std::chrono::system_clock::now();
    auto lastDay = std::chrono::system_clock::now();

    while (running) {
        char buffer[256];
        DWORD bytesRead;
        bool success = ReadFile(
            hPipe,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            NULL
        );

        if (success && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            float temperature = std::atof(buffer);
            if (temperature == 0.0f && buffer[0] != '0') {
                std::cerr << "Ошибка преобразования температуры: " << buffer << std::endl;
                continue;
            }

            std::string logMessage = getCurrentTime() + " - Температура: " + std::to_string(temperature);
            std::cout << "Получена температура: " << temperature << std::endl; // Отладочный вывод
            logTemperature(logMessage, "temperature.log");

            hourlyTemperatures.push_back(temperature);
            dailyTemperatures.push_back(temperature);

            auto now = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::hours>(now - lastHour).count() >= 1) {
                calculateHourlyAverage(hourlyTemperatures, "hourly_average.log");
                hourlyTemperatures.clear();
                lastHour = now;

                cleanOldLogs("hourly_average.log", 30 * 24 * 60 * 60);
            }

            if (std::chrono::duration_cast<std::chrono::hours>(now - lastDay).count() >= 24) {
                calculateDailyAverage(dailyTemperatures, "daily_average.log");
                dailyTemperatures.clear();
                lastDay = now;

                cleanOldLogs("daily_average.log", 365 * 24 * 60 * 60);
            }

            cleanOldLogs("temperature.log", 24 * 60 * 60);
        } else {
            std::cerr << "Ошибка чтения из канала! Код ошибки: " << GetLastError() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    CloseHandle(hPipe);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <имя канала>" << std::endl;
        return 1;
    }

    clearLogFile("temperature.log");
    clearLogFile("hourly_average.log");
    clearLogFile("daily_average.log");

    std::string pipeName = argv[1];

    std::thread inputThread(inputListener);

    // Задержка перед подключением к каналу
    std::cout << "Ожидание создания канала..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Задержка 5 секунд

    readTemperature(pipeName);

    inputThread.join();

    std::this_thread::sleep_for(std::chrono::seconds(5)); // Задержка перед завершением
    return 0;
}