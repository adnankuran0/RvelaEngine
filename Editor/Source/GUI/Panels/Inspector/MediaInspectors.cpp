#include "MediaInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Audio/AudioManager.h"
#include "GUI/AudioEffectUI.h"
#include "Scene/Entity.h"
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace rv {

void MediaInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<AudioEmitterComponent>("Audio Emitter", registry, selectedEntity, [](AudioEmitterComponent& emitter)
	{
		auto& audio = AudioManager::Get();

		std::string audioName = EditorUtils::GetAssetFileName(emitter.audioClipUUID);
		UI::DrawFullWidthAssetDropSlot("Audio Clip", audioName, [&](const std::string& pathStr)
		{
			if (pathStr.ends_with(".wav") || pathStr.ends_with(".mp3"))
			{
				AssetUUID audioClipUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
				emitter.audioClipUUID = audioClipUUID;
			}
		});

		ImGui::Spacing();

		if (UI::BeginPropertyTable("AudioClipTable"))
		{
			auto& busses = audio.GetBusses();
			AudioBus* currentBus = audio.GetBus(emitter.busID);
			if (!currentBus)
			{
				audio.SetBus(&emitter, 0);
				currentBus = audio.GetBus(emitter.busID);
			}

			std::string currentBusName = currentBus ? currentBus->GetName() : "Master";
			for (const auto& bus : busses)
			{
				if (bus->GetID() == emitter.busID)
				{
					currentBusName = bus->GetName();
					break;
				}
			}

			UI::PropertyLabel("Output Bus");
			if (ImGui::BeginCombo("##OutputBus", currentBusName.c_str()))
			{
				for (const auto& bus : busses)
				{
					bool isSelected = (emitter.busID == bus->GetID());
					if (ImGui::Selectable(bus->GetName().c_str(), isSelected))
						audio.SetBus(&emitter, bus->GetID());
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			UI::PropertyLabel("Playback");
			float playbackBtnWidth = (ImGui::GetContentRegionAvail().x - (2.0f * 4.0f)) / 3.0f;
			if (ImGui::Button("Play", ImVec2(playbackBtnWidth, 22))) audio.Play(&emitter);
			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button("Pause", ImVec2(playbackBtnWidth, 22))) audio.Pause(&emitter);
			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button("Stop", ImVec2(playbackBtnWidth, 22))) audio.Stop(&emitter);

			UI::PropertyLabel("Volume");
			float volume = emitter.volume;
			if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 2.0f, "%.2f")) audio.SetVolume(&emitter, volume);

			UI::PropertyLabel("Pitch");
			float pitch = emitter.pitch;
			if (ImGui::SliderFloat("##Pitch", &pitch, 0.01f, 4.0f, "%.2f")) audio.SetPitch(&emitter, pitch);

			UI::PropertyLabel("Loop");
			bool loop = emitter.loop;
			if (ImGui::Checkbox("##Loop", &loop)) audio.SetLoop(&emitter, loop);

			UI::PropertyLabel("Play On Create");
			ImGui::Checkbox("##PlayOnCreate", &emitter.playOnCreate);

			UI::PropertyLabel("Spatial Audio");
			bool spatial = emitter.spatial;
			if (ImGui::Checkbox("##Spatial", &spatial)) audio.SetSpatial(&emitter, spatial);

			UI::EndPropertyTable();
		}

		if (emitter.spatial)
		{
			if (ImGui::CollapsingHeader("Attenuation"))
			{
				if (UI::BeginPropertyTable("AttenuationTable"))
				{
					UI::PropertyLabel("Distance Model");
					const char* attenuationModels[] = { "None", "Inverse", "Linear", "Exponential" };
					int currentModel = static_cast<int>(emitter.attenuationModel);
					if (ImGui::Combo("##AttModel", &currentModel, attenuationModels, 4))
						audio.SetAttenuationModel(&emitter, static_cast<AttenuationModel>(currentModel));

					if (emitter.attenuationModel != AttenuationModel::None)
					{
						UI::PropertyLabel("Min Distance");
						float minDist = emitter.minDistance;
						if (ImGui::DragFloat("##MinDist", &minDist, 0.1f, 0.01f, emitter.maxDistance - 0.01f, "%.2f"))
							audio.SetMinDistance(&emitter, minDist);

						UI::PropertyLabel("Max Distance");
						float maxDist = emitter.maxDistance;
						if (ImGui::DragFloat("##MaxDist", &maxDist, 0.5f, emitter.minDistance + 0.01f, 10000.0f, "%.2f"))
							audio.SetMaxDistance(&emitter, maxDist);

						if (emitter.attenuationModel == AttenuationModel::Inverse || emitter.attenuationModel == AttenuationModel::Exponential)
						{
							UI::PropertyLabel("Rolloff");
							float rolloff = emitter.rolloff;
							if (ImGui::SliderFloat("##Rolloff", &rolloff, 0.0f, 10.0f, "%.2f"))
								audio.SetRolloff(&emitter, rolloff);
						}
					}
					UI::EndPropertyTable();
				}
			}

			if (ImGui::CollapsingHeader("Doppler Effect"))
			{
				if (UI::BeginPropertyTable("DopplerTable"))
				{
					UI::PropertyLabel("Enabled");
					bool dopplerEnabled = emitter.doppler;
					if (ImGui::Checkbox("##DopplerEnabled", &dopplerEnabled))
					{
						emitter.doppler = dopplerEnabled;
						audio.SetDopplerFactor(&emitter, dopplerEnabled ? emitter.dopplerFactor : 0.0f);
					}

					if (emitter.doppler)
					{
						UI::PropertyLabel("Factor");
						float dopplerFactor = emitter.dopplerFactor;
						if (ImGui::SliderFloat("##DopplerFactor", &dopplerFactor, 0.0f, 5.0f, "%.2f"))
							audio.SetDopplerFactor(&emitter, dopplerFactor);
					}
					UI::EndPropertyTable();
				}
			}
		}

		if (ImGui::CollapsingHeader("Audio Effects"))
		{
			UI::DrawAudioEffectsList("EmitterFX", emitter.effects,
				[&](std::shared_ptr<AudioEffect> fx) {
					audio.AddEmitterEffect(&emitter, fx);
				},
				[&](size_t index) {
					audio.RemoveEmitterEffect(&emitter, index);
				}
			);
		}
	});

	UI::DrawComponent<ParticleEmitterComponent>("Particle Emitter", registry, selectedEntity, [&](ParticleEmitterComponent& emitter)
	{
		if (UI::BeginPropertyTable("ParticleEmitterGeneralTable"))
		{
			UI::PropertyLabel("Emitting");
			bool emitting = emitter.emitting;
			if (ImGui::Checkbox("##Emitting", &emitting))
			{
				if (emitting)
				{
					if (emitter.oneShot && emitter.isFinished)
						scene.GetParticleSystem().ResetEmitter(selectedEntity);
					else
						emitter.emitting = true;
				}
				else
				{
					emitter.emitting = false;
				}
			}

			UI::PropertyLabel("Amount");
			int amount = static_cast<int>(emitter.amount);
			if (ImGui::DragInt("##Amount", &amount, 1.0f, 1, 100000))
				emitter.amount = static_cast<uint32_t>(std::max(1, amount));

			UI::PropertyLabel("Local Coords");
			ImGui::Checkbox("##LocalCoords", &emitter.localCoords);

			UI::EndPropertyTable();
		}

		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Time##ParticleTime"))
		{
			if (UI::BeginPropertyTable("ParticleTimeTable"))
			{
				UI::PropertyLabel("Lifetime");
				ImGui::DragFloat("##Lifetime", &emitter.lifetime, 0.05f, 0.01f, 100.0f, "%.2f s");

				UI::PropertyLabel("One Shot");
				ImGui::Checkbox("##OneShot", &emitter.oneShot);

				UI::PropertyLabel("Speed Scale");
				ImGui::DragFloat("##SpeedScale", &emitter.speedScale, 0.05f, 0.0f, 20.0f, "%.2f");

				UI::PropertyLabel("Explosiveness");
				ImGui::SliderFloat("##Explosiveness", &emitter.explosiveness, 0.0f, 1.0f, "%.2f");

				UI::PropertyLabel("Randomness");
				ImGui::SliderFloat("##Randomness", &emitter.randomness, 0.0f, 1.0f, "%.2f");

				UI::PropertyLabel("Lifetime Random");
				ImGui::SliderFloat("##LifetimeRandomness", &emitter.lifetimeRandomness, 0.0f, 1.0f, "%.2f");

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Emission Shape##ParticleShape"))
		{
			if (UI::BeginPropertyTable("ParticleShapeTable"))
			{
				UI::PropertyLabel("Shape");
				const char* shapeTypes[] = { "Point", "Sphere", "Sphere Surface", "Box" };
				int currentShape = static_cast<int>(emitter.emitterShape);
				if (ImGui::Combo("##Shape", &currentShape, shapeTypes, 4))
					emitter.emitterShape = static_cast<ParticleEmitterShape>(currentShape);

				if (emitter.emitterShape != ParticleEmitterShape::Point)
				{
					UI::DrawVec3Control("Extents", emitter.shapeDimensions, 1.0f, 0.05f, 0.001f, 1000.0f);
				}

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Direction & Velocity##ParticleDir"))
		{
			if (UI::BeginPropertyTable("ParticleDirTable"))
			{
				UI::DrawVec3Control("Direction", emitter.direction, 0.0f, 0.05f);

				UI::PropertyLabel("Spread");
				ImGui::SliderFloat("##Spread", &emitter.spread, 0.0f, 180.0f, "%.1f deg");

				UI::DrawVec3Control("Gravity", emitter.gravity, 0.0f, 0.1f);

				UI::PropertyLabel("Linear Vel Min");
				ImGui::DragFloat("##LinVelMin", &emitter.linearVelocityMin, 0.1f, -1000.0f, emitter.linearVelocityMax, "%.2f");

				UI::PropertyLabel("Linear Vel Max");
				ImGui::DragFloat("##LinVelMax", &emitter.linearVelocityMax, 0.1f, emitter.linearVelocityMin, 1000.0f, "%.2f");

				UI::PropertyLabel("Angular Vel Min");
				ImGui::DragFloat("##AngVelMin", &emitter.angularVelocityMin, 0.1f, -1000.0f, emitter.angularVelocityMax, "%.2f");

				UI::PropertyLabel("Angular Vel Max");
				ImGui::DragFloat("##AngVelMax", &emitter.angularVelocityMax, 0.1f, emitter.angularVelocityMin, 1000.0f, "%.2f");

				UI::PropertyLabel("Rotation Min");
				ImGui::DragFloat("##RotationMin", &emitter.rotationMin, 0.5f, -3600.0f, emitter.rotationMax, "%.1f deg");

				UI::PropertyLabel("Rotation Max");
				ImGui::DragFloat("##RotationMax", &emitter.rotationMax, 0.5f, emitter.rotationMin, 3600.0f, "%.1f deg");

				UI::PropertyLabel("Linear Accel Min");
				ImGui::DragFloat("##LinAccelMin", &emitter.linearAccelMin, 0.1f, -1000.0f, emitter.linearAccelMax, "%.2f");

				UI::PropertyLabel("Linear Accel Max");
				ImGui::DragFloat("##LinAccelMax", &emitter.linearAccelMax, 0.1f, emitter.linearAccelMin, 1000.0f, "%.2f");

				UI::PropertyLabel("Damping Min");
				ImGui::DragFloat("##DampingMin", &emitter.dampingMin, 0.01f, 0.0f, emitter.dampingMax, "%.2f");

				UI::PropertyLabel("Damping Max");
				ImGui::DragFloat("##DampingMax", &emitter.dampingMax, 0.01f, emitter.dampingMin, 1000.0f, "%.2f");

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Scale & Color##ParticleVisuals"))
		{
			if (UI::BeginPropertyTable("ParticleVisualsTable"))
			{
				UI::PropertyLabel("Scale Min");
				ImGui::DragFloat("##ScaleMin", &emitter.scaleMin, 0.05f, 0.0f, emitter.scaleMax, "%.2f");

				UI::PropertyLabel("Scale Max");
				ImGui::DragFloat("##ScaleMax", &emitter.scaleMax, 0.05f, emitter.scaleMin, 100.0f, "%.2f");

				UI::PropertyLabel("Scale End");
				ImGui::DragFloat("##ScaleEnd", &emitter.scaleEnd, 0.05f, 0.0f, 100.0f, "%.2f");

				UI::PropertyLabel("Start Color");
				float startCol[4] = { emitter.startColor.r, emitter.startColor.g, emitter.startColor.b, emitter.startColor.a };
				if (ImGui::ColorEdit4("##StartColor", startCol))
					emitter.startColor = glm::vec4(startCol[0], startCol[1], startCol[2], startCol[3]);

				UI::PropertyLabel("End Color");
				float endCol[4] = { emitter.endColor.r, emitter.endColor.g, emitter.endColor.b, emitter.endColor.a };
				if (ImGui::ColorEdit4("##EndColor", endCol))
					emitter.endColor = glm::vec4(endCol[0], endCol[1], endCol[2], endCol[3]);

				UI::EndPropertyTable();
			}
		}
	});

	UI::DrawComponent<DecalComponent>("Decal", registry, selectedEntity, [&](DecalComponent& decal)
	{
		std::string textureName = EditorUtils::GetAssetFileName(decal.textureUUID);
		UI::DrawFullWidthAssetDropSlot("Decal Texture Slot", textureName, [&](const std::string& pathStr)
		{
			static constexpr std::array exts = { ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".rtex" };
			bool isImage = std::any_of(exts.begin(), exts.end(), [&](const char* e) { return pathStr.ends_with(e); });
			if (isImage)
			{
				AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
				if (uuid.IsValid())
					decal.SetTexture(uuid);
			}
		});

		if (decal.textureUUID.IsValid())
		{
			if (ImGui::Button("Clear Texture", ImVec2(-FLT_MIN, 22)))
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Clear Decal Texture", [&]() {
					decal.ClearTexture();
				});
			}
		}

		if (UI::BeginPropertyTable("DecalTable"))
		{
			UI::PropertyLabel("Color");
			float col[4] = { decal.color.r, decal.color.g, decal.color.b, decal.color.a };
			if (ImGui::ColorEdit4("##DecalColor", col))
				decal.color = glm::vec4(col[0], col[1], col[2], col[3]);

			UI::PropertyLabel("Opacity");
			ImGui::SliderFloat("##DecalOpacity", &decal.opacity, 0.0f, 1.0f);

			UI::PropertyLabel("Lit");
			ImGui::Checkbox("##DecalLit", &decal.lit);

			if (decal.lit)
			{
				UI::PropertyLabel("Receive Shadows");
				ImGui::Checkbox("##DecalReceiveShadows", &decal.receiveShadows);
			}

			UI::PropertyLabel("Angle Cutoff");
			ImGui::SliderFloat("##DecalAngleCutoff", &decal.angleCutoff, 0.0f, 90.0f, "%.1f deg");

			UI::PropertyLabel("Upper Fade");
			ImGui::SliderFloat("##DecalUpperFade", &decal.upperFade, 0.0f, 0.5f, "%.2f");

			UI::PropertyLabel("Lower Fade");
			ImGui::SliderFloat("##DecalLowerFade", &decal.lowerFade, 0.0f, 0.5f, "%.2f");

			UI::PropertyLabel("Distance Fade");
			ImGui::Checkbox("##DecalDistanceFade", &decal.distanceFade);

			if (decal.distanceFade)
			{
				UI::PropertyLabel("Fade Start");
				ImGui::DragFloat("##DecalFadeStart", &decal.fadeStart, 0.5f, 0.0f, decal.fadeEnd, "%.1f m");

				UI::PropertyLabel("Fade End");
				ImGui::DragFloat("##DecalFadeEnd", &decal.fadeEnd, 0.5f, decal.fadeStart, 1000.0f, "%.1f m");
			}

			UI::PropertyLabel("Render Order");
			ImGui::DragInt("##DecalRenderOrder", &decal.renderOrder, 1, -100, 100);

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<AnimatorComponent>("Animator", registry, selectedEntity, [&](AnimatorComponent& animator)
	{
		std::string animLibName = EditorUtils::GetAssetFileName(animator.libraryUUID);
		UI::DrawFullWidthAssetDropSlot("Animation Library", animLibName, [&](const std::string& pathStr)
		{
			if (pathStr.ends_with(".ranimlib"))
			{
				AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
				if (uuid.IsValid())
				{
					animator.SetLibrary(uuid);
				}
			}
		});

		ImGui::Spacing();

		if (UI::BeginPropertyTable("AnimatorTable"))
		{
			if (animator.library && !animator.library->GetClips().empty())
			{
				UI::PropertyLabel("Active Clip");
				if (ImGui::BeginCombo("##ActiveClipCombo", animator.currentClipName.c_str()))
				{
					const auto& clips = animator.library->GetClips();

					std::vector<std::string> sortedNames;
					sortedNames.reserve(clips.size());
					for (const auto& [name, clip] : clips)
					{
						sortedNames.push_back(name);
					}
					std::sort(sortedNames.begin(), sortedNames.end());

					for (const auto& name : sortedNames)
					{
						bool isSelected = (animator.currentClipName == name);
						if (ImGui::Selectable(name.c_str(), isSelected))
						{
							animator.SetClip(name);
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			UI::PropertyLabel("Speed");
			ImGui::DragFloat("##PlaybackSpeed", &animator.playbackSpeed, 0.05f, -10.0f, 10.0f, "%.2fx");

			UI::PropertyLabel("Autoplay");
			ImGui::Checkbox("##AutoPlay", &animator.autoplay);

			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
