//
// Created by jcc on 24-8-1.
//

#ifndef CCLOG_LOG_H
#define CCLOG_LOG_H

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    enum OutputType {
        TO_CONSOLE,
        TO_FILE,
        TO_BOTH
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setOutput(OutputType type) {
        outputType = type;
    }

    void setLogLevel(LogLevel level) {
        logLevel = level;
    }

    void setLogFile(const std::string& filename) {
        logFileName = filename;
    }

    void setShowDebugInfo(bool show) {
        showDebugInfo = show;
    }

    template<typename... Args>
    void log(LogLevel level, const std::string& file, int line, const std::string& function, const std::string& format, Args... args) {
        if (level < logLevel) return;

        std::string message = formatMessage(format, args...);
        std::string output = currentDateTime() + " [" + levelToString(level) + "] " + message;

        if (showDebugInfo) {
            output += " (" + file + ":" + std::to_string(line) + " " + function + ")";
        }

        std::lock_guard<std::mutex> guard(mtx);

        if (outputType == TO_CONSOLE || outputType == TO_BOTH) {
            std::cout << colorCode(level) << output << "\033[0m" << std::endl;
        }

        if (outputType == TO_FILE || outputType == TO_BOTH) {
            std::ofstream logFile(logFileName, std::ios_base::app);
            if (logFile.is_open()) {
                logFile << output << std::endl;
            }
        }
    }

private:
    Logger() : logLevel(INFO), outputType(TO_CONSOLE), logFileName("log.txt"), showDebugInfo(false) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel logLevel;
    OutputType outputType;
    std::string logFileName;
    bool showDebugInfo;
    std::mutex mtx;

    std::string currentDateTime() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string levelToString(LogLevel level) {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::string colorCode(LogLevel level) {
        switch (level) {
            case DEBUG: return "\033[34m";   // Blue
            case INFO: return "\033[32m";    // Green
            case WARNING: return "\033[33m"; // Yellow
            case ERROR: return "\033[31m";   // Red
            default: return "\033[0m";       // Reset
        }
    }

    template<typename... Args>
    std::string formatMessage(const std::string& format, Args... args) {
        std::ostringstream oss;
        formatImpl(oss, format, args...);
        return oss.str();
    }

    template<typename T, typename... Args>
    void formatImpl(std::ostringstream& oss, const std::string& format, T value, Args... args) {
        size_t startPos = format.find("{}");
        size_t hexPos = format.find("{:X}");

        if (startPos == std::string::npos && hexPos == std::string::npos) {
            oss << format; // No more placeholders, append the rest of the string.
            return;
        }

        if (hexPos != std::string::npos && (startPos == std::string::npos || hexPos < startPos)) {
            oss << format.substr(0, hexPos) << std::hex << std::uppercase << value;
            formatImpl(oss, format.substr(hexPos + 4), args...); // Continue after {:X}
        } else {
            oss << format.substr(0, startPos) << value;
            formatImpl(oss, format.substr(startPos + 2), args...); // Continue after {}
        }
    }

    void formatImpl(std::ostringstream& oss, const std::string& format) {
        oss << format; // Base case for recursion, no more arguments to process.
    }
};

// Convenience macros for easier logging with file and line information
#define LOG_DEBUG(format, ...) Logger::getInstance().log(Logger::DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Logger::getInstance().log(Logger::INFO, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::getInstance().log(Logger::WARNING, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::getInstance().log(Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#endif //CCLOG_LOG_H
