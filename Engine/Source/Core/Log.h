#pragma once
#include "plog/Log.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include <string>

class MinimalFormatter
{
public:
    static plog::util::nstring header()
    {
        return plog::util::nstring(); // Header istemiyoruz
    }

    static plog::util::nstring format(const plog::Record& record)
    {
        plog::util::nostringstream ss;

        switch (record.getSeverity())
        {
        case plog::fatal:   ss << "[FATAL] "; break;
        case plog::error:   ss << "[ERROR] "; break;
        case plog::warning: ss << "[WARN ] "; break;
        case plog::info:    ss << "[INFO ] "; break;
        case plog::debug:   ss << "[DEBUG] "; break;
        case plog::verbose: ss << "[TRACE] "; break;
        default:            ss << "[UNKWN] "; break;
        }

        ss << record.getMessage() << "\n";
        return ss.str();
    }
};

class RvelaLog
{
public:
    static void Init(const std::string& filePath);
};


