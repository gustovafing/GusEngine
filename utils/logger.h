#pragma once
#include <string>

using namespace std;
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
    private:
    void Write(LogLevel lvl, string msg, string source = "");

    static string GetLevelLabel(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
        }
        return "UNKNOWN";
    }
    public:
    inline void Debug(string msg) { Write(LogLevel::DEBUG, msg); }
    inline void Debug(string source, string msg) { Write(LogLevel::DEBUG, msg, source); }
    inline void Info(string msg) { Write(LogLevel::INFO, msg); }
    inline void Info(string source, string msg) { Write(LogLevel::INFO, msg, source); }
    inline void Warn(string msg) { Write(LogLevel::WARN, msg); }
    inline void Warn(string source, string msg) { Write(LogLevel::WARN, msg, source); }
    inline void Error(string msg) { Write(LogLevel::ERROR, msg); }
    inline void Error(string source, string msg) { Write(LogLevel::ERROR, msg, source); }
    inline void FatalError(string msg) { Write(LogLevel::FATAL, msg); }
    inline void FatalError(string source, string msg) { Write(LogLevel::FATAL, msg, source); }
};