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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

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
        logFile << message << std::endl;
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
}

void inputListener() {
    char input;
    while (running) {
        std::cin >> input;
        if (input == 'q') {
            running = false;
            std::cout << "Завершение работы программы..." << std::endl;
        }
    }
}

void readTemperature(const std::string& port) {
#ifdef _WIN32
    HANDLE hPipe = CreateFile(
        port.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия канала на Windows! Код ошибки: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Канал успешно открыт: " << port << std::endl;
#else
    int fd = open(port.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Ошибка открытия порта на macOS/Linux!" << std::endl;
        return;
    }

    std::cout << "Порт успешно открыт: " << port << std::endl;
#endif

    std::vector<float> hourlyTemperatures;
    std::vector<float> dailyTemperatures;
    auto lastHour = std::chrono::system_clock::now();
    auto lastDay = std::chrono::system_clock::now();

    while (running) {
        char buffer[256];
#ifdef _WIN32
        DWORD bytesRead;
        bool success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
        if (!success) {
            std::cerr << "Ошибка чтения из канала на Windows! Код ошибки: " << GetLastError() << std::endl;
            break;
        }
#else
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            std::cerr << "Ошибка чтения из порта на macOS/Linux!" << std::endl;
            break;
        }
#endif

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            float temperature = std::atof(buffer);
            std::string logMessage = getCurrentTime() + " - Температура: " + std::to_string(temperature);
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
        }

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

    clearLogFile("temperature.log");
    clearLogFile("hourly_average.log");
    clearLogFile("daily_average.log");

    std::string port = argv[1];

    std::thread inputThread(inputListener);
    readTemperature(port);

    inputThread.join();
    return 0;
}
