#pragma once

#include "../HAL/HAL.h"

static constexpr bool LogInfo = true;
static constexpr bool LogError = true;
static constexpr bool LogWarning = true;
static constexpr bool LogDebug = true;
static constexpr bool LogCustom = true;
static constexpr bool LogRaw = true;

class Log {
   public:
    static void Info(const char* message);
    static void Error(const char* message);
    static void Warning(const char* message);
    static void Debug(const char* message);
    static void Custom(const char* level, const char* message);
    static void Raw(const char* str);
};

inline void Log::Info(const char* message) {
    if constexpr (LogInfo) {
        static const char* level = "INFO";
        HAL::Print(level);
        HAL::Print(": ");
        HAL::Print(message);
        HAL::Print("\n");
    }
}

inline void Log::Error(const char* message) {
    if constexpr (LogError) {
        static const char* level = "ERROR";
        HAL::Print(level);
        HAL::Print(": ");
        HAL::Print(message);
        HAL::Print("\n");
    }
}

inline void Log::Warning(const char* message) {
    if constexpr (LogWarning) {
        static const char* level = "WARNING";
        HAL::Print(level);
        HAL::Print(": ");
        HAL::Print(message);
        HAL::Print("\n");
    }
}

inline void Log::Debug(const char* message) {
    if constexpr (LogDebug) {
        static const char* level = "DEBUG";
        HAL::Print(level);
        HAL::Print(": ");
        HAL::Print(message);
        HAL::Print("\n");
    }
}

inline void Log::Custom(const char* level, const char* message) {
    if constexpr (LogCustom) {
        HAL::Print(level);
        HAL::Print(": ");
        HAL::Print(message);
        HAL::Print("\n");
    }
}
inline void Log::Raw(const char* str) {
    if constexpr (LogRaw) {
        HAL::Print(str);
    }
}
