#pragma once
#include <string>
#include <iostream>
#include <ostream>
#include "Command.h"
class Outputer {
public:
    class ErrorStream {
        Command m_Cmd;
    public:
        explicit ErrorStream(const Command& cmd) : m_Cmd(cmd) {
            std::cout << '[' << m_Cmd.GetVerb() << "] Error: ";
        }

        ~ErrorStream() {
            std::cout << std::endl;
        }

        template<typename T>
        ErrorStream& operator<<(const T& value) {
            std::cout << value;
            return *this;
        }
    };
    class InfoStream {
        Command m_Cmd;
    public:
        explicit InfoStream(const Command& cmd) : m_Cmd(cmd) {
            std::cout << '[' << m_Cmd.GetVerb() << "] ";
        }
        InfoStream() = default;

        ~InfoStream() {
            std::cout << std::endl;
        }

        template<typename T>
        InfoStream& operator<<(const T& value) {
            std::cout << value;
            return *this;
        }
    };

    static ErrorStream ErrorLn(const Command& cmd) { return ErrorStream(cmd); }
    static InfoStream InfoLn() { return {}; }
    static InfoStream InfoLn(const Command& cmd) { return InfoStream(cmd); }
    static std::ostream& Out() { return std::cout; }
};