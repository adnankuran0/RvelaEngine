#include "rvelapch.h"
#include "Window.h"
#include "Core/Log.h"
#include "Core/VFS.h"
#include "Event/EventManager.h"
#include <stb_image.h>
#include "GLFW/glfw3.h"
#include "GLAD/gl.h"
#include "Event/KeyEvents.h"
#include "Event/MouseEvents.h"
#include "Event/WindowEvents.h"
#include "Utils/FileUtils.h"
#include "Utils/ProjectManager.h"
#include "Input/Input.h"
#include "glm/vec2.hpp"
#include "Core/ProjectSettings.h"
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

using namespace rv;

Window::Window() noexcept
{
}

Window::Window(const WindowData& data) noexcept
    : m_WindowData(data)
{
}

Window::Window(const std::string& title, int width, int height, bool maximized) noexcept
    : m_WindowData(title, width, height, maximized)
{
}

Window::~Window()
{
    ResetCursor();
    ResetIcon();
}

Window::Window(Window&& other) noexcept
    : m_Window(other.m_Window)
    , m_CustomCursor(other.m_CustomCursor)
#if defined(_WIN32)
    , m_CustomHIcon(other.m_CustomHIcon)
#endif
    , m_WindowData(std::move(other.m_WindowData))
    , m_VSync(other.m_VSync)
    , m_Borderless(other.m_Borderless)
    , m_Resizable(other.m_Resizable)
{
    other.m_Window = nullptr;
    other.m_CustomCursor = nullptr;
#if defined(_WIN32)
    other.m_CustomHIcon = nullptr;
#endif
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_Window = other.m_Window;
        m_CustomCursor = other.m_CustomCursor;
#if defined(_WIN32)
        m_CustomHIcon = other.m_CustomHIcon;
#endif
        m_WindowData = std::move(other.m_WindowData);
        m_VSync = other.m_VSync;
        m_Borderless = other.m_Borderless;
        m_Resizable = other.m_Resizable;

        other.m_Window = nullptr;
        other.m_CustomCursor = nullptr;
#if defined(_WIN32)
        other.m_CustomHIcon = nullptr;
#endif
    }
    return *this;
}

void Window::SetCallbacks() noexcept
{
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, MouseMovedCallback);
    glfwSetScrollCallback(m_Window, MouseScrolledCallback);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
    glfwSetWindowFocusCallback(m_Window, WindowFocusCallback);
    glfwSetDropCallback(m_Window, FileDropCallback);
    glfwSetCharCallback(m_Window, CharCallback);
}

void Window::Init()
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_MAXIMIZED, m_WindowData.maximized ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_WindowData.size.width, m_WindowData.size.height, m_WindowData.title.c_str(), nullptr, nullptr);
    if (!m_Window) 
    {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    if (!m_WindowData.maximized)
    {
        CenterOnMonitor();
    }

    if (!SetIcon(EDITOR_PATH("icon.png").GetAbsoluteStr()))
    {
        if (!SetIcon("icon.png"))
        {
            SetIcon("Assets/icon.png");
        }
    }
        
    
    glfwMakeContextCurrent(m_Window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) 
    {
        
        LOG_ERROR("Failed to initialize GLAD");
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    LOG_INFO("OpenGL Version: {}", (glVersion ? reinterpret_cast<const char*>(glVersion) : "Unknown"));

    glfwSwapInterval(m_VSync ? 1 : 0);

    SetCallbacks();

    glfwShowWindow(m_Window);
}

Window Window::Create() noexcept
{
    Window win;
    win.Init();
    return win;
}

Window Window::Create(const std::string& title, int width, int height, bool maximized) noexcept
{
    Window win{ title, width, height, maximized };
    win.Init();
    return win;
}

void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
{
    if (action == GLFW_PRESS) 
    {
        KeyCode keyCode = static_cast<KeyCode>(key);
        EventManager::PushEvent(new KeyPressedEvent(keyCode));
    }
    else if (action == GLFW_RELEASE) 
    {
        KeyCode keyCode = static_cast<KeyCode>(key);
        EventManager::PushEvent(new KeyReleasedEvent(keyCode));
    }
}

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept 
{
    if (action == GLFW_PRESS) 
    {
        MouseCode mouseCode = static_cast<MouseCode>(button);
        EventManager::PushEvent(new MouseButtonPressedEvent(mouseCode));
    }
    else if (action == GLFW_RELEASE) 
    {
        MouseCode mouseCode = static_cast<MouseCode>(button);
        EventManager::PushEvent(new MouseButtonReleasedEvent(mouseCode));
    }
}

void Window::MouseMovedCallback(GLFWwindow* window, double xpos, double ypos) noexcept 
{
    EventManager::PushEvent(new MouseMovedEvent(static_cast<float>(xpos), static_cast<float>(ypos)));
}

void Window::MouseScrolledCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept 
{
    Input::OnMouseScrolled(static_cast<float>(xoffset), static_cast<float>(yoffset));
    EventManager::PushEvent(new MouseScrolledEvent(static_cast<float>(xoffset), static_cast<float>(yoffset)));
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept 
{
    glViewport(0, 0, width, height);
    EventManager::PushEvent(new WindowResizedEvent(width, height));
}

void Window::WindowFocusCallback(GLFWwindow* window, int focused) noexcept
{
    const bool isFocused = (focused == GLFW_TRUE);
    Input::OnFocusChanged(isFocused);
    EventManager::PushEvent(new WindowFocusEvent(isFocused));
}

void Window::FileDropCallback(GLFWwindow* window, int pathCount, const char* paths[]) noexcept
{
#if defined(_WIN32)
    POINT pt;
    ::GetCursorPos(&pt);
    EventManager::PushEvent(new FileDroppedEvent(pathCount, paths, static_cast<float>(pt.x), static_cast<float>(pt.y)));
#else
    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    EventManager::PushEvent(new FileDroppedEvent(pathCount, paths, static_cast<float>(xpos), static_cast<float>(ypos)));
#endif
}

void Window::CharCallback(GLFWwindow* window, unsigned int codepoint) noexcept
{
    Input::OnCharTyped(codepoint);
}

WindowSize Window::GetSize() noexcept
{
    if (!m_Window) {
        return WindowSize{ 0, 0 };
    }
    glfwGetWindowSize(m_Window, &m_WindowData.size.width, &m_WindowData.size.height);
    return m_WindowData.size;
}

void Window::SetTitle(const std::string& title)
{
    m_WindowData.title = title;
    if (m_Window)
    {
        glfwSetWindowTitle(m_Window, title.c_str());
    }
}

void Window::SetSize(int width, int height)
{
    if (!m_Window) return;
    m_WindowData.size.width = width;
    m_WindowData.size.height = height;
    glfwSetWindowSize(m_Window, width, height);
}

void Window::SetPosition(int x, int y)
{
    if (!m_Window) return;
    glfwSetWindowPos(m_Window, x, y);
}

void Window::CenterOnMonitor()
{
    if (!m_Window) return;
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    if (!mon) return;

    int mx, my, mw, mh;
    glfwGetMonitorWorkarea(mon, &mx, &my, &mw, &mh);

    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    glfwSetWindowPos(m_Window, mx + (mw - w) / 2, my + (mh - h) / 2);
}

glm::ivec2 Window::GetPosition() const
{
    int x = 0, y = 0;
    if (m_Window) glfwGetWindowPos(m_Window, &x, &y);
    return { x, y };
}

bool Window::IsMaximized() const
{
    return m_Window && glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void Window::Maximize()
{
    if (m_Window) glfwMaximizeWindow(m_Window);
}

void Window::Restore()
{
    if (m_Window) glfwRestoreWindow(m_Window);
}

void Window::Shutdown()
{
    ResetCursor();
    ResetIcon();
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void Window::SetVSync(bool vsync)
{
    m_VSync = vsync;
    if (m_Window)
    {
        glfwSwapInterval(vsync ? 1 : 0);
    }
}

void Window::SetResizable(bool resizable)
{
    m_Resizable = resizable;
    if (m_Window)
    {
        glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }
}

void Window::SetBorderless(bool borderless)
{
    m_Borderless = borderless;
    if (m_Window)
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, borderless ? GLFW_FALSE : GLFW_TRUE);
    }
}

void Window::SetWindowMode(WindowMode mode)
{
    if (!m_Window) return;

    switch (mode)
    {
    case WindowMode::Fullscreen:
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor)
        {
            const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
            if (vidMode)
            {
                glfwSetWindowMonitor(m_Window, monitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
            }
        }
        break;
    }
    case WindowMode::Maximized:
    {
        GLFWmonitor* curMonitor = glfwGetWindowMonitor(m_Window);
        if (curMonitor)
        {
            glfwSetWindowMonitor(m_Window, nullptr, 100, 100, m_WindowData.size.width, m_WindowData.size.height, 0);
        }
        glfwMaximizeWindow(m_Window);
        break;
    }
    case WindowMode::Minimized:
    {
        glfwIconifyWindow(m_Window);
        break;
    }
    case WindowMode::Windowed:
    default:
    {
        GLFWmonitor* curMonitor = glfwGetWindowMonitor(m_Window);
        if (curMonitor)
        {
            glfwSetWindowMonitor(m_Window, nullptr, 100, 100, m_WindowData.size.width, m_WindowData.size.height, 0);
            CenterOnMonitor();
        }
        glfwRestoreWindow(m_Window);
        break;
    }
    }
}

void Window::ApplySettings(const ProjectSettings& settings)
{
    if (!m_Window) return;

    uint32_t targetW = settings.overrideWindowSize ? settings.windowOverrideWidth : settings.windowWidth;
    uint32_t targetH = settings.overrideWindowSize ? settings.windowOverrideHeight : settings.windowHeight;
    if (targetW > 0 && targetH > 0)
    {
        SetSize(static_cast<int>(targetW), static_cast<int>(targetH));
        CenterOnMonitor();
    }

    SetResizable(settings.resizable);
    SetBorderless(settings.borderless);
    SetWindowMode(settings.windowMode);
    SetVSync(settings.vsync);

    if (!settings.iconPath.empty())
    {
        SetIcon(settings.iconPath);
    }
    if (!settings.cursorIconPath.empty())
    {
        SetCursorIcon(settings.cursorIconPath, settings.cursorHotspotX, settings.cursorHotspotY);
    }
}

bool Window::SetIcon(const std::string& iconPath)
{
    if (!m_Window || iconPath.empty())
        return false;

    if (VFS::Get().IsMounted())
    {
        std::string norm = VFS::NormalizePath(iconPath);
        if (VFS::Get().FileExists(norm))
        {
            std::vector<uint8_t> buffer;
            if (VFS::Get().ReadFile(norm, buffer) && !buffer.empty())
            {
                return SetIconFromMemory(buffer.data(), buffer.size());
            }
        }
    }

    std::filesystem::path resolved;
    if (std::filesystem::exists(iconPath))
    {
        resolved = iconPath;
    }
    else if (ProjectManager::IsProjectLoaded() && std::filesystem::exists(ProjectManager::GetProjectPath() / iconPath))
    {
        resolved = ProjectManager::GetProjectPath() / iconPath;
    }
    else if (std::filesystem::exists(Path::GetExecutableDirectory() / iconPath))
    {
        resolved = Path::GetExecutableDirectory() / iconPath;
    }
    else if (EDITOR_PATH(iconPath).Exists())
    {
        resolved = EDITOR_PATH(iconPath).GetAbsolute();
    }
    else if (ENGINE_PATH(iconPath).Exists())
    {
        resolved = ENGINE_PATH(iconPath).GetAbsolute();
    }

    if (resolved.empty())
    {
        return false;
    }

#if defined(_WIN32)
    std::string ext = resolved.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".ico")
    {
        HWND hwnd = glfwGetWin32Window(m_Window);
        if (hwnd)
        {
            HICON hIcon = (HICON)LoadImageA(nullptr, resolved.string().c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            if (hIcon)
            {
                if (m_CustomHIcon)
                {
                    DestroyIcon((HICON)m_CustomHIcon);
                }
                m_CustomHIcon = hIcon;
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                return true;
            }
        }
    }
#endif

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(resolved.string().c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        return false;
    }

    GLFWimage images[1];
    images[0].width = width;
    images[0].height = height;
    images[0].pixels = pixels;
    glfwSetWindowIcon(m_Window, 1, images);
    stbi_image_free(pixels);
    return true;
}

bool Window::SetIconFromMemory(const uint8_t* data, size_t size)
{
    if (!m_Window || !data || size == 0)
        return false;

#if defined(_WIN32)
    if (size > 4 && data[0] == 0 && data[1] == 0 && data[2] == 1 && data[3] == 0)
    {
        HWND hwnd = glfwGetWin32Window(m_Window);
        if (hwnd)
        {
            HICON hIcon = CreateIconFromResourceEx((PBYTE)data, (DWORD)size, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
            if (hIcon)
            {
                if (m_CustomHIcon)
                {
                    DestroyIcon((HICON)m_CustomHIcon);
                }
                m_CustomHIcon = hIcon;
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                return true;
            }
        }
    }
#endif

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels)
    {
        LOG_WARN("Window::SetIconFromMemory: Failed to decode icon image from memory");
        return false;
    }

    GLFWimage images[1];
    images[0].width = width;
    images[0].height = height;
    images[0].pixels = pixels;
    glfwSetWindowIcon(m_Window, 1, images);
    stbi_image_free(pixels);
    return true;
}

void Window::ResetIcon()
{
    if (m_Window)
    {
        glfwSetWindowIcon(m_Window, 0, nullptr);
    }
#if defined(_WIN32)
    if (m_CustomHIcon)
    {
        DestroyIcon((HICON)m_CustomHIcon);
        m_CustomHIcon = nullptr;
    }
#endif
}

bool Window::SetCursorIcon(const std::string& cursorPath, int hotX, int hotY)
{
    if (cursorPath.empty())
    {
        ResetCursor();
        return true;
    }

    if (VFS::Get().IsMounted())
    {
        std::string norm = VFS::NormalizePath(cursorPath);
        if (VFS::Get().FileExists(norm))
        {
            std::vector<uint8_t> buffer;
            if (VFS::Get().ReadFile(norm, buffer) && !buffer.empty())
            {
                return SetCursorIconFromMemory(buffer.data(), buffer.size(), hotX, hotY);
            }
        }
    }

    std::filesystem::path resolved;
    if (std::filesystem::exists(cursorPath))
    {
        resolved = cursorPath;
    }
    else if (ProjectManager::IsProjectLoaded() && std::filesystem::exists(ProjectManager::GetProjectPath() / cursorPath))
    {
        resolved = ProjectManager::GetProjectPath() / cursorPath;
    }
    else if (std::filesystem::exists(Path::GetExecutableDirectory() / cursorPath))
    {
        resolved = Path::GetExecutableDirectory() / cursorPath;
    }
    else if (EDITOR_PATH(cursorPath).Exists())
    {
        resolved = EDITOR_PATH(cursorPath).GetAbsolute();
    }
    else if (ENGINE_PATH(cursorPath).Exists())
    {
        resolved = ENGINE_PATH(cursorPath).GetAbsolute();
    }

    if (resolved.empty())
    {
        LOG_WARN("Window::SetCursorIcon: File not found: {}", cursorPath);
        return false;
    }

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(resolved.string().c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        LOG_WARN("Window::SetCursorIcon: Failed to load cursor image: {}", resolved.string());
        return false;
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;

    GLFWcursor* newCursor = glfwCreateCursor(&image, hotX, hotY);
    stbi_image_free(pixels);

    if (!newCursor)
    {
        LOG_WARN("Window::SetCursorIcon: glfwCreateCursor failed for {}", resolved.string());
        return false;
    }

    if (m_CustomCursor)
    {
        glfwDestroyCursor(m_CustomCursor);
    }
    m_CustomCursor = newCursor;
    if (m_Window)
    {
        glfwSetCursor(m_Window, m_CustomCursor);
    }
    return true;
}

bool Window::SetCursorIconFromMemory(const uint8_t* data, size_t size, int hotX, int hotY)
{
    if (!m_Window || !data || size == 0)
        return false;

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels)
    {
        LOG_WARN("Window::SetCursorIconFromMemory: Failed to decode cursor image from memory");
        return false;
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;

    GLFWcursor* newCursor = glfwCreateCursor(&image, hotX, hotY);
    stbi_image_free(pixels);

    if (!newCursor)
    {
        LOG_WARN("Window::SetCursorIconFromMemory: glfwCreateCursor failed");
        return false;
    }

    if (m_CustomCursor)
    {
        glfwDestroyCursor(m_CustomCursor);
    }
    m_CustomCursor = newCursor;
    if (m_Window)
    {
        glfwSetCursor(m_Window, m_CustomCursor);
    }
    return true;
}

void Window::SetStandardCursor(CursorShape shape)
{
    if (!m_Window)
        return;

    int glfwShape = GLFW_ARROW_CURSOR;
    switch (shape)
    {
    case CursorShape::Arrow:     glfwShape = GLFW_ARROW_CURSOR; break;
    case CursorShape::IBeam:     glfwShape = GLFW_IBEAM_CURSOR; break;
    case CursorShape::Crosshair: glfwShape = GLFW_CROSSHAIR_CURSOR; break;
    case CursorShape::Hand:      glfwShape = GLFW_HAND_CURSOR; break;
    case CursorShape::HResize:   glfwShape = GLFW_HRESIZE_CURSOR; break;
    case CursorShape::VResize:   glfwShape = GLFW_VRESIZE_CURSOR; break;
    default:                     glfwShape = GLFW_ARROW_CURSOR; break;
    }

    GLFWcursor* newCursor = glfwCreateStandardCursor(glfwShape);
    if (!newCursor)
        return;

    if (m_CustomCursor)
    {
        glfwDestroyCursor(m_CustomCursor);
    }
    m_CustomCursor = newCursor;
    glfwSetCursor(m_Window, m_CustomCursor);
}

void Window::ResetCursor()
{
    if (m_Window)
    {
        glfwSetCursor(m_Window, nullptr);
    }
    if (m_CustomCursor)
    {
        glfwDestroyCursor(m_CustomCursor);
        m_CustomCursor = nullptr;
    }
}

