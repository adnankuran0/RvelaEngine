#pragma once
#include <string>
#include <vector>
#include <memory>

namespace rv {

class Scene;

class Command
{
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetName() const = 0;
    virtual void SetScene(Scene* scene) {}
};

class CompoundCommand : public Command
{
public:
    CompoundCommand(std::string name = "Compound Action")
        : m_Name(std::move(name)) {}

    void Add(std::unique_ptr<Command> command)
    {
        if (command)
            m_Commands.push_back(std::move(command));
    }

    void Execute() override
    {
        for (auto& cmd : m_Commands)
        {
            if (cmd)
                cmd->Execute();
        }
    }

    void Undo() override
    {
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it)
        {
            if (*it)
                (*it)->Undo();
        }
    }

    void SetScene(Scene* scene) override
    {
        for (auto& cmd : m_Commands)
        {
            if (cmd)
                cmd->SetScene(scene);
        }
    }

    std::string GetName() const override { return m_Name; }
    size_t GetCount() const { return m_Commands.size(); }
    bool IsEmpty() const { return m_Commands.empty(); }

private:
    std::string m_Name;
    std::vector<std::unique_ptr<Command>> m_Commands;
};

}
