#include "rvelapch.h"
#include "Log.h"
#include "EditorConsoleSink.h"

using namespace rv;

std::shared_ptr<spdlog::logger> RvelaLog::s_Logger;

void RvelaLog::Init(const std::string&)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%^%l%$] %v");

    auto editorSink = EditorConsoleSink::GetShared();
    editorSink->set_pattern("[%T] [%^%l%$] %v");

    std::vector<spdlog::sink_ptr> sinks{ consoleSink, editorSink };

    s_Logger = std::make_shared<spdlog::logger>("Rvela", sinks.begin(), sinks.end());
    s_Logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(s_Logger);
}

std::shared_ptr<spdlog::logger>& RvelaLog::GetLogger()
{
    return s_Logger;
}