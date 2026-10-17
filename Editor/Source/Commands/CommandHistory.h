#pragma once
#include "Command.h"
#include "Core/Engine.h"
#include <vector>
#include <memory>
#include <string>

namespace rv {

class CommandHistory
{
public:
    static CommandHistory& Get()
    {
        static CommandHistory s_Instance;
        return s_Instance;
    }

    void SetCurrentScene(Scene* scene)
    {
        for (auto& cmd : m_Commands)
        {
            if (cmd)
                cmd->SetScene(scene);
        }
    }

    void Execute(std::unique_ptr<Command> command)
    {
        if (!command) return;

        command->Execute();

        // If we were in the middle of undo stack and did a new action, discard future redo history
        if (m_CurrentIndex < m_Commands.size())
        {
            m_Commands.erase(m_Commands.begin() + m_CurrentIndex, m_Commands.end());
        }

        m_Commands.push_back(std::move(command));

        if (m_Commands.size() > m_MaxHistorySize)
        {
            m_Commands.erase(m_Commands.begin());
        }
        else
        {
            m_CurrentIndex++;
        }
    }

    bool CanUndo() const
    {
        return m_CurrentIndex > 0;
    }

    bool CanRedo() const
    {
        return m_CurrentIndex < m_Commands.size();
    }

    void Undo()
    {
        if (!CanUndo()) return;

        if (Engine::Get())
            SetCurrentScene(&Engine::Get()->GetActiveScene());

        m_CurrentIndex--;
        m_Commands[m_CurrentIndex]->Undo();
    }

    void Redo()
    {
        if (!CanRedo()) return;

        if (Engine::Get())
            SetCurrentScene(&Engine::Get()->GetActiveScene());

        m_Commands[m_CurrentIndex]->Execute();
        m_CurrentIndex++;
    }

    void Clear()
    {
        m_Commands.clear();
        m_CurrentIndex = 0;
    }

    std::string GetUndoName() const
    {
        if (CanUndo())
            return m_Commands[m_CurrentIndex - 1]->GetName();
        return "";
    }

    std::string GetRedoName() const
    {
        if (CanRedo())
            return m_Commands[m_CurrentIndex]->GetName();
        return "";
    }

    void SetMaxHistorySize(size_t size) { m_MaxHistorySize = size; }
    size_t GetMaxHistorySize() const { return m_MaxHistorySize; }

private:
    CommandHistory() = default;

    std::vector<std::unique_ptr<Command>> m_Commands;
    size_t m_CurrentIndex = 0;
    size_t m_MaxHistorySize = 100;
};

}
