#include "rvelapch.h"
#include "Log.h"

namespace rv { 

std::shared_ptr<spdlog::logger> RvelaLog::s_Logger;

void RvelaLog::Init(const std::string&)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%^%l%$] %v");

    s_Logger = std::make_shared<spdlog::logger>(
        "Rvela",
        consoleSink
    );

    s_Logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(s_Logger);
}


std::shared_ptr<spdlog::logger>& RvelaLog::GetLogger()
{
    return s_Logger;
}

}