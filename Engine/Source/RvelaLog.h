#pragma once
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include  <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <string>

class RvelaLog
{
public:
    static void Init(const std::string& filePath);
};


