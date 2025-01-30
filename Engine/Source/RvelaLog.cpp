#include "rvelapch.h"
#include "RvelaLog.h"



void RvelaLog::Init(const std::string& filePath)
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender(filePath.c_str(), 1024 * 1024, 5);

    plog::init(plog::debug, &fileAppender).addAppender(&consoleAppender);
}



