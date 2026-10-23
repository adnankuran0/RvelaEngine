#include "rvelapch.h"
#include "Log.h"
#include "EditorConsoleSink.h"

using namespace rv;

std::shared_ptr<spdlog::logger> RvelaLog::s_Logger;

void RvelaLog::Init(const std::string& filePath)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%^%l%$] %v");

    auto editorSink = EditorConsoleSink::GetShared();
    editorSink->set_pattern("[%T] [%^%l%$] %v");

    std::vector<spdlog::sink_ptr> sinks{ consoleSink, editorSink };

    if (!filePath.empty())
    {
        try
        {
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filePath, 1024 * 1024 * 5, 3);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks.push_back(fileSink);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cerr << "Failed to initialize file log sink: " << ex.what() << std::endl;
        }
    }

    s_Logger = std::make_shared<spdlog::logger>("Rvela", sinks.begin(), sinks.end());
    s_Logger->set_level(spdlog::level::trace);
    s_Logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(s_Logger);
}

std::shared_ptr<spdlog::logger>& RvelaLog::GetLogger()
{
    return s_Logger;
}
