#include "ImGuiLayer.h"

ImGuiLayer::ImGuiLayer(Engine* engine)
    : m_Engine(engine)
{
}

ImGuiLayer::~ImGuiLayer()
{
}

void ImGuiLayer::OnAttach()
{
    // ImGui'nin baþlatýlmasý
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // GLFW ve OpenGL baðdaþtýrýcýlarý
    ImGui_ImplGlfw_InitForOpenGL(m_Engine->GetWindow()->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Tema ayarlarý
    ImGui::StyleColorsDark();
}

void ImGuiLayer::OnDetach()
{
    // ImGui'nin temizlenmesi
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate()
{
    // Yeni bir frame baþlatýlmasý
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::OnRender()
{
    // Basit bir arayüz örneði
    ImGui::Begin("Engine Debug");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    if (ImGui::Button("Exit Engine"))
    {
        glfwSetWindowShouldClose(m_Engine->GetWindow()->GetGLFWWindow(), true);
    }

    ImGui::End();

    // Frame'in render edilmesi
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnEvent()
{
    // Olaylarýn iþlenmesi
    // Þimdilik boþ, gerekirse motorunun event sistemi ile baðlayabilirsin
}
