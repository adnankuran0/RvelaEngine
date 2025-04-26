#include "rvelapch.h"
#include "Window.h"
#include "RvelaLog.h"
#include "Event/EventManager.h"

/**
 * @brief Constructs a Window with default settings.
 *
 * Initializes the window using the default WindowData settings.
 */
Window::Window() noexcept
{
	Init();
}

/**
 * @brief Constructs a Window with the specified settings.
 *
 * @param title The title of the window.
 * @param width The width of the window.
 * @param height The height of the window.
 */
Window::Window(const std::string& title, int width, int height) noexcept
    :m_WindowData(title, width, height)
{
	Init();
}

/**
 * @brief Destroys the Window and releases its resources.
 */
Window::~Window()
{
	Shutdown();
}

/*
*@brief Sets GLFW callback functions to use in event system.
*/
void Window::SetCallbacks() noexcept
{
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, MouseMovedCallback);
    glfwSetScrollCallback(m_Window, MouseScrolledCallback);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
}

/**
 * @brief Initializes the GLFW window and sets up event callbacks.
 *
 * Initializes GLFW and GLEW, creates the window, sets up the OpenGL context,
 * and registers event callbacks for keyboard, mouse, and window events.
 */
void Window::Init()
{
    if (!glfwInit()) {
        LOG_ERROR << "Failed to initialize GLFW";
        throw std::runtime_error("Failed to initialize GLFW");
    }

    LOG_INFO << "GLFW Initialized";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_WindowData.size.width, m_WindowData.size.height, m_WindowData.title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        LOG_ERROR << "Failed to create GLFW window";
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_Window);
    if (glewInit() != GLEW_OK) {
        LOG_ERROR << "Failed to initialize GLEW";
        throw std::runtime_error("Failed to initialize GLEW");
    }

    LOG_INFO << "GLEW Initialized";
    const GLubyte* glVersion = glGetString(GL_VERSION);
    LOG_INFO << "OpenGL Version: " << (glVersion ? reinterpret_cast<const char*>(glVersion) : "Unknown");

    SetCallbacks();
}

/**
 * @brief Creates a Window with default settings.
 *
 * @return Window A new Window instance with default settings.
 */
Window Window::Create() noexcept
{
    return Window{};
}

/**
 * @brief Creates a Window with the specified settings.
 *
 * @param title The title of the window.
 * @param width The width of the window.
 * @param height The height of the window.
 * @return Window A new Window instance with the specified settings.
 */
Window Window::Create(const std::string& title, int width, int height) noexcept
{
    return Window{ title, width, height };
}

/**
 * @brief Callback for keyboard events.
 *
 * Dispatches KeyPressedEvent or KeyReleasedEvent to the EventManager based on the action.
 *
 * @param window The GLFW window that received the event.
 * @param key The keyboard key that was pressed or released.
 * @param scancode The system-specific scancode of the key.
 * @param action The key action (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT).
 * @param mods Bit field describing which modifier keys were held down.
 */
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

/**
 * @brief Callback for mouse button events.
 *
 * Dispatches MouseButtonPressedEvent or MouseButtonReleasedEvent to the EventManager.
 *
 * @param window The GLFW window that received the event.
 * @param button The mouse button that was pressed or released.
 * @param action The button action (GLFW_PRESS, GLFW_RELEASE).
 * @param mods Bit field describing which modifier keys were held down.
 */
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

/**
 * @brief Callback for mouse movement events.
 *
 * Dispatches a MouseMovedEvent to the EventManager with the new cursor position.
 *
 * @param window The GLFW window that received the event.
 * @param xpos The new x-coordinate of the cursor.
 * @param ypos The new y-coordinate of the cursor.
 */
void Window::MouseMovedCallback(GLFWwindow* window, double xpos, double ypos) noexcept {
    EventManager::PushEvent(new MouseMovedEvent(static_cast<float>(xpos), static_cast<float>(ypos)));
}

/**
 * @brief Callback for mouse scroll events.
 *
 * Dispatches a MouseScrolledEvent to the EventManager with the scroll offset.
 *
 * @param window The GLFW window that received the event.
 * @param xoffset The scroll offset in the x direction.
 * @param yoffset The scroll offset in the y direction.
 */
void Window::MouseScrolledCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept {
    EventManager::PushEvent(new MouseScrolledEvent(static_cast<float>(xoffset), static_cast<float>(yoffset)));
}

/**
 * @brief Callback for framebuffer size change events.
 *
 * Updates the OpenGL viewport and dispatches a WindowResizedEvent to the EventManager.
 *
 * @param window The GLFW window that received the event.
 * @param width The new width of the framebuffer.
 * @param height The new height of the framebuffer.
 */
void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept {
    glViewport(0, 0, width, height);
    EventManager::PushEvent(new WindowResizedEvent(width, height));
}
