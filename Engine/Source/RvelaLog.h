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

#define LOG_DEBUG PLOGD
#define LOG_INFO PLOGI
#define LOG_WARNING PLOGW
#define LOG_ERROR PLOGE
#define LOG_FATAL PLOGF
