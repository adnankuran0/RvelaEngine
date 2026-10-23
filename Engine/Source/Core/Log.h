#pragma once
#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define LOG_TRACE(...)    ::rv::RvelaLog::GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::rv::RvelaLog::GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::rv::RvelaLog::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::rv::RvelaLog::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::rv::RvelaLog::GetLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)    ::rv::RvelaLog::GetLogger()->critical(__VA_ARGS__)

namespace rv { 

class RvelaLog
{
public:
    static void Init(const std::string& filePath);
    static std::shared_ptr<spdlog::logger>& GetLogger();

private:
    static std::shared_ptr<spdlog::logger> s_Logger;
};

}