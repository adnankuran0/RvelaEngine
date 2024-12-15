#include "Window.h"

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
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    std::cout << "GLFW Initialized" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(_WIN32)
    // Add Windows-specific configuration if needed
#elif defined(__linux__)
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "ferx");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "Ferx");
#endif

    m_Window = glfwCreateWindow(m_WindowData.size.width, m_WindowData.size.height, m_WindowData.title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_Window);
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
    }
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