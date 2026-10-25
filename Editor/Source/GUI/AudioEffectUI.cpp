#include "AudioEffectUI.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "GUI/Style.h"
#include "Panels/Inspector/InspectorUIUtils.h"
#include <cstdio>

namespace rv::UI {

void DrawAudioEffectDetails(AudioEffect* effect)
{
    if (!effect) return;

    switch (effect->GetType())
    {
    case AudioEffectType::Reverb:
    {
        auto* rv = static_cast<AudioReverbEffect*>(effect);
        if (UI::BeginPropertyTable("ReverbTable"))
        {
            UI::PropertyLabel("Room Size");
            ImGui::SliderFloat("##RoomSize", &rv->roomSize, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Damping");
            ImGui::SliderFloat("##Damping", &rv->damping, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Width");
            ImGui::SliderFloat("##Width", &rv->width, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Wet Level");
            ImGui::SliderFloat("##Wet", &rv->wet, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Dry Level");
            ImGui::SliderFloat("##Dry", &rv->dry, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Freeze Mode");
            ImGui::Checkbox("##Freeze", &rv->freeze);

            UI::EndPropertyTable();
        }
        break;
    }

    case AudioEffectType::Limiter:
    {
        auto* lm = static_cast<AudioLimiterEffect*>(effect);
        if (UI::BeginPropertyTable("LimiterTable"))
        {
            UI::PropertyLabel("Threshold");
            ImGui::SliderFloat("##Threshold", &lm->thresholdDb, -30.0f, 0.0f, "%.1f dB");

            UI::PropertyLabel("Ceiling");
            ImGui::SliderFloat("##Ceiling", &lm->ceilingDb, -12.0f, 0.0f, "%.1f dB");

            UI::PropertyLabel("Release");
            ImGui::SliderFloat("##Release", &lm->releaseMs, 1.0f, 1000.0f, "%.0f ms");

            UI::PropertyLabel("Makeup Gain");
            ImGui::SliderFloat("##MakeupGain", &lm->makeupGainDb, 0.0f, 24.0f, "%.1f dB");

            UI::PropertyLabel("Reduction");
            float grLinear = std::clamp(lm->currentGainReduction, 0.0f, 1.0f);
            float grDb = (grLinear > 1e-4f) ? (20.0f * std::log10(grLinear)) : -40.0f;
            char grText[32];
            std::snprintf(grText, sizeof(grText), "%.1f dB", grDb);

            float reductionAmount = 1.0f - grLinear; // 0 = no reduction, 1 = max reduction
            ImVec4 barColor = (reductionAmount > 0.4f) ? GetErrorColor() : (reductionAmount > 0.1f ? GetWarningColor() : GetSuccessColor());
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            ImGui::ProgressBar(reductionAmount, ImVec2(-FLT_MIN, 18.0f), grText);
            ImGui::PopStyleColor();

            UI::EndPropertyTable();
        }
        break;
    }

    case AudioEffectType::Distortion:
    {
        auto* ds = static_cast<AudioDistortionEffect*>(effect);
        if (UI::BeginPropertyTable("DistortionTable"))
        {
            UI::PropertyLabel("Type");
            const char* distTypes[] = { "Soft Clip", "Hard Clip", "Tape / Tanh", "Foldback", "Bitcrush" };
            int currentType = static_cast<int>(ds->type);
            if (ImGui::Combo("##DistType", &currentType, distTypes, IM_ARRAYSIZE(distTypes)))
                ds->type = static_cast<DistortionType>(currentType);

            UI::PropertyLabel("Drive");
            ImGui::SliderFloat("##Drive", &ds->drive, 1.0f, 50.0f, "%.1fx");

            UI::PropertyLabel("Mix");
            ImGui::SliderFloat("##Mix", &ds->mix, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Tone");
            ImGui::SliderFloat("##Tone", &ds->tone, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Output Gain");
            ImGui::SliderFloat("##OutGain", &ds->outputGain, 0.0f, 2.0f, "%.2f");

            if (ds->type == DistortionType::Bitcrush)
            {
                UI::PropertyLabel("Bit Depth");
                ImGui::SliderInt("##Bits", &ds->bits, 2, 16, "%d bits");
            }

            UI::EndPropertyTable();
        }
        break;
    }

    case AudioEffectType::Delay:
    {
        auto* dl = static_cast<AudioDelayEffect*>(effect);
        if (UI::BeginPropertyTable("DelayTable"))
        {
            UI::PropertyLabel("Time");
            ImGui::SliderFloat("##DelayTime", &dl->delayTime, 0.01f, 2.0f, "%.2f s");

            UI::PropertyLabel("Feedback");
            ImGui::SliderFloat("##Feedback", &dl->feedback, 0.0f, 0.95f, "%.2f");

            UI::PropertyLabel("Mix");
            ImGui::SliderFloat("##Mix", &dl->mix, 0.0f, 1.0f, "%.2f");

            UI::PropertyLabel("Ping-Pong");
            ImGui::Checkbox("##PingPong", &dl->pingPong);

            UI::EndPropertyTable();
        }
        break;
    }

    case AudioEffectType::Filter:
    {
        auto* fl = static_cast<AudioFilterEffect*>(effect);
        if (UI::BeginPropertyTable("FilterTable"))
        {
            UI::PropertyLabel("Type");
            const char* filterTypes[] = { "Low Pass", "High Pass", "Band Pass", "Notch" };
            int currentType = static_cast<int>(fl->type);
            if (ImGui::Combo("##FilterType", &currentType, filterTypes, IM_ARRAYSIZE(filterTypes)))
                fl->type = static_cast<FilterType>(currentType);

            UI::PropertyLabel("Cutoff");
            ImGui::SliderFloat("##Cutoff", &fl->cutoffFrequency, 20.0f, 20000.0f, "%.0f Hz", ImGuiSliderFlags_Logarithmic);

            UI::PropertyLabel("Resonance (Q)");
            ImGui::SliderFloat("##Resonance", &fl->resonance, 0.1f, 10.0f, "%.2f");

            UI::EndPropertyTable();
        }
        break;
    }

    case AudioEffectType::Equalizer:
    {
        auto* eq = static_cast<AudioEqualizerEffect*>(effect);
        if (UI::BeginPropertyTable("EqualizerTable"))
        {
            UI::PropertyLabel("Low Gain");
            ImGui::SliderFloat("##LowGain", &eq->lowGainDb, -24.0f, 24.0f, "%.1f dB");

            UI::PropertyLabel("Mid Gain");
            ImGui::SliderFloat("##MidGain", &eq->midGainDb, -24.0f, 24.0f, "%.1f dB");

            UI::PropertyLabel("High Gain");
            ImGui::SliderFloat("##HighGain", &eq->highGainDb, -24.0f, 24.0f, "%.1f dB");

            UI::PropertyLabel("Low Cutoff");
            ImGui::DragFloat("##LowCutoff", &eq->lowCutoff, 5.0f, 20.0f, 1000.0f, "%.0f Hz");

            UI::PropertyLabel("High Cutoff");
            ImGui::DragFloat("##HighCutoff", &eq->highCutoff, 20.0f, 1000.0f, 20000.0f, "%.0f Hz");

            UI::EndPropertyTable();
        }
        break;
    }
    }
}

void DrawAudioEffectsList(
    const std::string& id,
    std::vector<std::shared_ptr<AudioEffect>>& effects,
    const std::function<void(std::shared_ptr<AudioEffect>)>& onAddEffect,
    const std::function<void(size_t)>& onRemoveEffect,
    const std::function<void(size_t, size_t)>& onMoveEffect)
{
    ImGui::PushID(id.c_str());

    ImGui::PushStyleColor(ImGuiCol_Button, GetAccentColor());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetAccentHoverColor());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetAccentActiveColor());

    if (ImGui::Button("+ Add Effect", ImVec2(-FLT_MIN, 26.0f)))
    {
        ImGui::OpenPopup("AddEffectPopup");
    }
    ImGui::PopStyleColor(3);

    if (ImGui::BeginPopup("AddEffectPopup"))
    {
        auto addEffect = [&](AudioEffectType type)
        {
            auto fx = CreateAudioEffect(type);
            if (fx)
            {
                if (onAddEffect) onAddEffect(fx);
                else effects.push_back(fx);
            }
            ImGui::CloseCurrentPopup();
        };

        if (ImGui::MenuItem("Reverb", ""))
            addEffect(AudioEffectType::Reverb);

        if (ImGui::MenuItem("Limiter", ""))
            addEffect(AudioEffectType::Limiter);

        if (ImGui::MenuItem("Distortion", ""))
            addEffect(AudioEffectType::Distortion);

        if (ImGui::MenuItem("Delay", ""))
            addEffect(AudioEffectType::Delay);

        if (ImGui::MenuItem("Filter", ""))
            addEffect(AudioEffectType::Filter);

        if (ImGui::MenuItem("Equalizer", ""))
            addEffect(AudioEffectType::Equalizer);

        ImGui::EndPopup();
    }

    if (effects.empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No effects attached.");
        ImGui::PopID();
        return;
    }

    ImGui::Spacing();

    size_t effectToRemove = SIZE_MAX;
    size_t moveFrom = SIZE_MAX;
    size_t moveTo = SIZE_MAX;

    for (size_t i = 0; i < effects.size(); ++i)
    {
        auto& fx = effects[i];
        if (!fx) continue;

        ImGui::PushID(static_cast<int>(i));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.18f, 0.24f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.24f, 0.24f, 0.32f, 0.8f));

        std::string headerId = std::string(fx->GetTypeName()) + "##EffectHeader" + std::to_string(i);

        bool open = ImGui::CollapsingHeader(headerId.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float btnWidth = 20.0f;
        float totalBtnsWidth = (btnWidth * 3.0f) + 24.0f + (itemSpacing * 4.0f);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - totalBtnsWidth + 20.0f);

        ImGui::Checkbox("##Enabled", &fx->enabled);

        ImGui::SameLine();
        if (ImGui::Button("^", ImVec2(btnWidth, 20.0f)) && i > 0)
        {
            moveFrom = i;
            moveTo = i - 1;
        }

        ImGui::SameLine();
        if (ImGui::Button("v", ImVec2(btnWidth, 20.0f)) && (i + 1 < effects.size()))
        {
            moveFrom = i;
            moveTo = i + 1;
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetErrorColor());
        if (ImGui::Button("X", ImVec2(btnWidth, 20.0f)))
        {
            effectToRemove = i;
        }
        ImGui::PopStyleColor();

        if (open)
        {
            ImGui::Indent(8.0f);
            ImGui::Spacing();
            DrawAudioEffectDetails(fx.get());
            ImGui::Spacing();
            ImGui::Unindent(8.0f);
            ImGui::Separator();
        }

        ImGui::PopID();
    }

    if (effectToRemove != SIZE_MAX)
    {
        if (onRemoveEffect) onRemoveEffect(effectToRemove);
        else effects.erase(effects.begin() + effectToRemove);
    }
    else if (moveFrom != SIZE_MAX && moveTo != SIZE_MAX)
    {
        if (onMoveEffect)
        {
            onMoveEffect(moveFrom, moveTo);
        }
        else
        {
            auto elem = effects[moveFrom];
            effects.erase(effects.begin() + moveFrom);
            effects.insert(effects.begin() + moveTo, elem);
        }
    }

    ImGui::PopID();
}

}
