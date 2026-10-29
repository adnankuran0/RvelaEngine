#include "NavigationInspectors.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"
#include "Scene/Components/NavMeshSurfaceComponent.h"
#include "Scene/Components/NavMeshAgentComponent.h"
#include "Scene/Components/NavMeshObstacleComponent.h"

namespace rv {

void NavigationInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
    entt::registry& registry = scene.GetRegistry();

    UI::DrawComponent<NavMeshSurfaceComponent>("NavMesh Surface", registry, selectedEntity, [&](NavMeshSurfaceComponent& ns)
    {
        if (ns.IsBaked())
        {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Status: Baked (Ready)");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Status: Not Baked");
        }

        ImGui::Spacing();

        if (ImGui::Button("Bake NavMesh", ImVec2(-FLT_MIN, 28.0f)))
        {
            ns.Bake(scene);
        }

        if (ns.IsBaked())
        {
            if (ImGui::Button("Clear NavMesh", ImVec2(-FLT_MIN, 22.0f)))
            {
                ns.Clear();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (UI::BeginPropertyTable("NavMeshSurfaceProps"))
        {
            UI::PropertyLabel("Agent Radius");
            ImGui::SliderFloat("##AgentRadius", &ns.config.agentRadius, 0.1f, 3.0f, "%.2f m");

            UI::PropertyLabel("Agent Height");
            ImGui::SliderFloat("##AgentHeight", &ns.config.agentHeight, 0.5f, 5.0f, "%.2f m");

            UI::PropertyLabel("Max Step Climb");
            ImGui::SliderFloat("##AgentMaxClimb", &ns.config.agentMaxClimb, 0.05f, 2.0f, "%.2f m");

            UI::PropertyLabel("Max Slope Angle");
            ImGui::SliderFloat("##AgentMaxSlope", &ns.config.agentMaxSlope, 0.0f, 75.0f, "%.1f deg");

            UI::PropertyLabel("Cell Size (Voxel XZ)");
            ImGui::SliderFloat("##CellSize", &ns.config.cellSize, 0.05f, 1.0f, "%.2f m");

            UI::PropertyLabel("Cell Height (Voxel Y)");
            ImGui::SliderFloat("##CellHeight", &ns.config.cellHeight, 0.05f, 1.0f, "%.2f m");

            UI::PropertyLabel("Filter Low Obstacles");
            ImGui::Checkbox("##FilterLowHanging", &ns.config.filterLowHangingObstacles);

            UI::PropertyLabel("Auto Bake on Play");
            ImGui::Checkbox("##AutoBake", &ns.autoBakeOnPlay);

            UI::PropertyLabel("Draw Debug Mesh");
            ImGui::Checkbox("##DrawDebug", &ns.drawDebugMesh);

            if (ns.drawDebugMesh)
            {
                UI::PropertyLabel("Debug Color");
                ImGui::ColorEdit4("##DebugColor", &ns.debugColor.x, ImGuiColorEditFlags_NoInputs);
            }

            UI::EndPropertyTable();
        }
    });

    UI::DrawComponent<NavMeshAgentComponent>("NavMesh Agent", registry, selectedEntity, [&](NavMeshAgentComponent& agent)
    {
        if (agent.isMoving)
        {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "State: Moving (Remaining: %.2f m)", agent.RemainingDistance());
        }
        else if (agent.isStopped)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "State: Stopped");
        }
        else if (agent.HasReachedDestination())
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "State: Reached Destination");
        }
        else
        {
            ImGui::TextDisabled("State: Idle");
        }

        ImGui::Spacing();

        if (UI::BeginPropertyTable("NavMeshAgentProps"))
        {
            UI::PropertyLabel("Speed");
            ImGui::SliderFloat("##Speed", &agent.speed, 0.1f, 30.0f, "%.1f m/s");

            UI::PropertyLabel("Angular Speed");
            ImGui::SliderFloat("##AngularSpeed", &agent.angularSpeed, 10.0f, 720.0f, "%.0f deg/s");

            UI::PropertyLabel("Acceleration");
            ImGui::SliderFloat("##Acceleration", &agent.acceleration, 0.5f, 50.0f, "%.1f");

            UI::PropertyLabel("Stopping Distance");
            ImGui::SliderFloat("##StoppingDistance", &agent.stoppingDistance, 0.05f, 5.0f, "%.2f m");

            UI::PropertyLabel("Radius");
            ImGui::SliderFloat("##AgentRad", &agent.radius, 0.1f, 2.0f, "%.2f m");

            UI::PropertyLabel("Height");
            ImGui::SliderFloat("##AgentH", &agent.height, 0.5f, 4.0f, "%.2f m");

            UI::PropertyLabel("Update Rotation");
            ImGui::Checkbox("##UpdateRot", &agent.updateRotation);

            UI::PropertyLabel("Update Position");
            ImGui::Checkbox("##UpdatePos", &agent.updatePosition);

            UI::PropertyLabel("Auto Repath");
            ImGui::Checkbox("##AutoRepath", &agent.autoRepath);

            if (agent.autoRepath)
            {
                UI::PropertyLabel("Repath Interval");
                ImGui::SliderFloat("##RepathInterval", &agent.repathInterval, 0.1f, 5.0f, "%.2f s");
            }

            UI::PropertyLabel("Draw Debug Path");
            ImGui::Checkbox("##DrawDebugPath", &agent.drawDebugPath);

            UI::EndPropertyTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Destination Test:");
        ImGui::DragFloat3("##DestTarget", &agent.destination.x, 0.1f);
        if (ImGui::Button("Go To Destination", ImVec2(-FLT_MIN, 24.0f)))
        {
            agent.SetDestination(agent.destination);
        }

        if (agent.isMoving)
        {
            if (ImGui::Button("Stop Agent", ImVec2(-FLT_MIN, 22.0f)))
            {
                agent.Stop();
            }
        }
        else if (agent.isStopped)
        {
            if (ImGui::Button("Resume Agent", ImVec2(-FLT_MIN, 22.0f)))
            {
                agent.Resume();
            }
        }
    });

    UI::DrawComponent<NavMeshObstacleComponent>("NavMesh Obstacle", registry, selectedEntity, [&](NavMeshObstacleComponent& obs)
    {
        if (UI::BeginPropertyTable("NavMeshObstacleProps"))
        {
            UI::PropertyLabel("Affect NavMesh");
            ImGui::Checkbox("##AffectNavMesh", &obs.affectNavMesh);

            UI::PropertyLabel("Is Walkable Surface");
            ImGui::Checkbox("##IsWalkable", &obs.isWalkable);

            UI::EndPropertyTable();
        }
    });
}

}
