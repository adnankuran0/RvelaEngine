#include "Window.h"
#include "RvelaLog.h"
#include "Event/EventManager.h"

Window::Window()
{
	Init();
}

Window::Window(const std::string& title, int width, int height)
{
	m_WindowData = WindowData{ title, width, height };
	Init();
}

Window::~Window()
{
	Shutdown();
}

void Window::Init()
{
    if (!glfwInit()) {
        LOG_ERROR << "Failed to initialize GLFW";
        return;
    }

    LOG_INFO << "GLFW Initialized";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    m_Window = glfwCreateWindow(m_WindowData.size.width, m_WindowData.size.height, m_WindowData.title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        LOG_ERROR << "Failed to create GLFW window";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_Window);
    if (glewInit() != GLEW_OK)
    {
        LOG_ERROR << "Failed to initialize GLEW";
    }
    else
    {
        LOG_INFO << "GLEW Initialized";
    }
    LOG_INFO << glGetString(GL_VERSION);

    glfwSetKeyCallback(m_Window, m_KeyCallback);
}

Window Window::Create()
{
    return Window{};
}

Window Window::Create(const std::string& title, int width, int height)
{
    return Window{ title, width, height };
}

GLFWwindow* Window::GetWindow() const
{
    return m_Window;
}

const std::string& Window::GetTitle() const
{
    return m_WindowData.title;
}

WindowSize Window::GetSize()
{
    glfwGetWindowSize(m_Window, &m_WindowData.size.width, &m_WindowData.size.height);
    return m_WindowData.size;
}

void Window::Shutdown() const
{
    glfwDestroyWindow(m_Window);
}

void Window::m_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        KeyCode keyCode = static_cast<KeyCode>(key);
        KeyPressedEvent* event = new KeyPressedEvent(keyCode);
        EventManager::pushEvent(event); // Event'i EventManager'a ekliyoruz
    }
}