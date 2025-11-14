// Logging.h

#pragma once
#include <fstream>
#include <sstream>

#include "Command.h"
#include "Core.h"

std::string GetTimestamp(
    std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now());

class Logger {
public:
    explicit Logger(const std::string& logOutPath);
    ~Logger();
    void Log(const Command& command);
    void Show() const;
    std::string GetBuffer() const { return buffer.str(); }
    void Save();

private:
    std::string m_FilePath;
    std::stringstream buffer;
    std::ofstream m_LoggingOut;
};