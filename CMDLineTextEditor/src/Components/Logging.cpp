#include "Logging.h"

#include <filesystem>
#include <fstream>

#include "Outputer.h"

std::string GetTimestamp(std::chrono::time_point<std::chrono::system_clock> t) {
    const auto timeTick = std::chrono::system_clock::to_time_t(t);

    const std::tm tm = *std::localtime(&timeTick);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d %H:%M:%S");
    return ss.str();
}

Logger::Logger(const std::string& logOutPath) {
    m_FilePath = logOutPath;
    buffer << "session start at " << GetTimestamp() << std::endl;
}
Logger::~Logger() {
    try {
        Save();
    }
    catch (const std::exception& e) {
        Outputer::InfoLn() << "Failed to save" << this->m_FilePath << ": " << e.what();
    }
}

void Logger::Log(const Command& command) {

    buffer << GetTimestamp(command.GetTime()) << ' ' << command.GetLine() << std::endl;
    buffer.flush();
}

void Logger::Show() const {
    Outputer::Out() << buffer.str();
}

void Logger::Save() {
    if (!m_LoggingOut.is_open()) {
        std::filesystem::path fp(m_FilePath);
        if (!fp.parent_path().empty()) {
            if (!std::filesystem::exists(fp.parent_path())) {
                std::filesystem::create_directories(fp.parent_path());
            }
            if (!std::filesystem::is_directory(fp.parent_path())) {
                throw std::runtime_error("There is a file named `data`!");
            }
        }
        m_LoggingOut = std::ofstream(m_FilePath, std::ios::app | std::ios::out);
        if (!m_LoggingOut.is_open())
            throw std::runtime_error("Could not open file: " + m_FilePath);
        if (!m_LoggingOut.is_open()) {
            throw std::runtime_error("Logging output file not opened");
        }
    }
    m_LoggingOut << buffer.str();
    m_LoggingOut.close();
}