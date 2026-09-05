#pragma once
#include "spdlog/sinks/base_sink.h"
#include <mutex>
#include <vector>
#include <string>
#include <functional>

namespace rv {

enum class LogLevel : uint8_t
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Lua
};

struct LogMessage
{
    std::string text;
    LogLevel level;
    uint32_t count = 1;
};

class EditorConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    static std::shared_ptr<EditorConsoleSink> GetShared()
    {
        static auto instance = std::make_shared<EditorConsoleSink>();
        return instance;
    }

    static EditorConsoleSink& Get()
    {
        return *GetShared();
    }

    void LogLua(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_LuaCount++;

        if (!m_Messages.empty() && m_Messages.back().text == text && m_Messages.back().level == LogLevel::Lua)
        {
            m_Messages.back().count++;
            return;
        }

        if (m_Messages.size() >= 1500)
            m_Messages.erase(m_Messages.begin());

        m_Messages.push_back({ text, LogLevel::Lua, 1 });
    }

    void ForEachMessage(const std::function<void(const std::vector<LogMessage>&)>& fn)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        fn(m_Messages);
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Messages.clear();
        m_InfoCount = m_WarnCount = m_ErrorCount = m_LuaCount = 0;
    }

    uint32_t GetInfoCount() const { return m_InfoCount; }
    uint32_t GetWarnCount() const { return m_WarnCount; }
    uint32_t GetErrorCount() const { return m_ErrorCount; }
    uint32_t GetLuaCount() const { return m_LuaCount; }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

        LogLevel level = LogLevel::Info;
        switch (msg.level)
        {
        case spdlog::level::trace:    level = LogLevel::Trace; break;
        case spdlog::level::debug:    level = LogLevel::Debug; break;
        case spdlog::level::info:     level = LogLevel::Info; break;
        case spdlog::level::warn:     level = LogLevel::Warn; break;
        case spdlog::level::err:      level = LogLevel::Error; break;
        case spdlog::level::critical: level = LogLevel::Critical; break;
        default: break;
        }

        std::string textStr = fmt::to_string(formatted);

        std::lock_guard<std::mutex> lock(m_Mutex);

        if (level == LogLevel::Warn) m_WarnCount++;
        else if (level == LogLevel::Error || level == LogLevel::Critical) m_ErrorCount++;
        else m_InfoCount++;

        if (!m_Messages.empty() && m_Messages.back().text == textStr && m_Messages.back().level == level)
        {
            m_Messages.back().count++;
            return;
        }

        if (m_Messages.size() >= 1500)
            m_Messages.erase(m_Messages.begin());

        m_Messages.push_back({ std::move(textStr), level, 1 });
    }

    void flush_() override {}

private:
    std::mutex m_Mutex;
    std::vector<LogMessage> m_Messages;
    uint32_t m_InfoCount = 0;
    uint32_t m_WarnCount = 0;
    uint32_t m_ErrorCount = 0;
    uint32_t m_LuaCount = 0;
};

} // namespace rv