#include "rvelapch.h"
#include "Log.h"


std::shared_ptr<spdlog::logger> RvelaLog::s_Logger;

void RvelaLog::Init(const std::string& filePath)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%^%l%$] %v");

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filePath, 1024 * 1024, 5);

    fileSink->set_pattern("[%l] %v");

    std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };

    s_Logger = std::make_shared<spdlog::logger>("Rvela", sinks.begin(), sinks.end());
    s_Logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(s_Logger);
}

std::shared_ptr<spdlog::logger>& RvelaLog::GetLogger()
{
    return s_Logger;
}



