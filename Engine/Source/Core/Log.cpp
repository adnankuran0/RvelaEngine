#include "rvelapch.h"
#include "Log.h"



void RvelaLog::Init(const std::string& filePath)
{
    static plog::ColorConsoleAppender<MinimalFormatter> consoleAppender;
    static plog::RollingFileAppender<MinimalFormatter> fileAppender(filePath.c_str(), 1024 * 1024, 5);

    plog::init(plog::debug, &fileAppender).addAppender(&consoleAppender);
}



