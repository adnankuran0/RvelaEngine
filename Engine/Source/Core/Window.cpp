#include "rvelapch.h"
#include "Window.h"
#include "Core/Log.h"
#include "Event/EventManager.h"
#include <stb_image.h>

Window::Window() noexcept
{
}

Window::Window(const std::string& title, int width, int height) noexcept
    :m_WindowData(title, width, height)
{
	Init();
}

Window::~Window()
{
}

void Window::SetCallbacks() noexcept
{
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, MouseMovedCallback);
    glfwSetScrollCallback(m_Window, MouseScrolledCallback);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
}

void Window::Init()
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_WindowData.size.width, m_WindowData.size.height, m_WindowData.title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    GLFWimage images[1];
    images[0].pixels = stbi_load("C:\\RvelaEngine\\Binaries\\windows-x86_64\\Release\\Editor\\icon.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(m_Window, 1, images);
    stbi_image_free(images[0].pixels);

    glfwMakeContextCurrent(m_Window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        
        LOG_ERROR("Failed to initialize GLAD");
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    LOG_INFO("OpenGL Version: {}", (glVersion ? reinterpret_cast<const char*>(glVersion) : "Unknown"));

    SetCallbacks();
}

Window Window::Create() noexcept
{
    return Window{};
}

Window Window::Create(const std::string& title, int width, int height) noexcept
{
    return Window{ title, width, height };
}

void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
{
    if (action == GLFW_PRESS) {
        KeyCode keyCode = static_cast<KeyCode>(key);
        EventManager::PushEvent(new KeyPressedEvent(keyCode));
    }
    else if (action == GLFW_RELEASE) {
        KeyCode keyCode = static_cast<KeyCode>(key);
        EventManager::PushEvent(new KeyReleasedEvent(keyCode));
    }
}

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept {
    if (action == GLFW_PRESS) {
        MouseCode mouseCode = static_cast<MouseCode>(button);
        EventManager::PushEvent(new MouseButtonPressedEvent(mouseCode));
    }
    else if (action == GLFW_RELEASE) {
        MouseCode mouseCode = static_cast<MouseCode>(button);
        EventManager::PushEvent(new MouseButtonReleasedEvent(mouseCode));
    }
}

void Window::MouseMovedCallback(GLFWwindow* window, double xpos, double ypos) noexcept {
    EventManager::PushEvent(new MouseMovedEvent(static_cast<float>(xpos), static_cast<float>(ypos)));
}

void Window::MouseScrolledCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept {
    EventManager::PushEvent(new MouseScrolledEvent(static_cast<float>(xoffset), static_cast<float>(yoffset)));
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept {
    glViewport(0, 0, width, height);
    EventManager::PushEvent(new WindowResizedEvent(width, height));
}
