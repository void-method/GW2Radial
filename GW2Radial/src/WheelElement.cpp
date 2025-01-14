#include <WheelElement.h>
#include <Core.h>
#include <UnitQuad.h>
#include <Utility.h>
#include <Wheel.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Effect.h>

#include "../shaders/registers.h"

namespace GW2Radial
{

WheelElement::WheelElement(uint id, const std::string &nickname, const std::string &category,
							const std::string &displayName, IDirect3DDevice9* dev, ComPtr<IDirect3DTexture9> tex)
	: nickname_(nickname), displayName_(displayName), elementId_(id),
	  isShownOption_(displayName + " Visible", nickname + "_visible", category, true),
	  sortingPriorityOption_(displayName + " Priority", nickname + "_priority", category, int(id)),
	  keybind_(nickname, displayName, category),
	  appearance_(tex)
{
	if(tex == nullptr)
	    appearance_ = CreateTextureFromResource(dev, Core::i().dllModule(), elementId_);

	GW2_ASSERT(appearance_ != nullptr);

	D3DSURFACE_DESC desc;
	appearance_->GetLevelDesc(0, &desc);
	aspectRatio_ = float(desc.Height) / float(desc.Width);
	texWidth_ = float(desc.Width);
}

int WheelElement::DrawPriority(int extremumIndicator)
{
	ImVec4 col = ConvertVector(color());
	ImGui::PushStyleColor(ImGuiCol_Text, col);

	ImGuiConfigurationWrapper(&ImGui::Checkbox, ("##Displayed" + nickname_).c_str(), isShownOption_);
	ImGui::SameLine();
	if(!isShownOption_.value() || !isActive())
		ImGui::PushFont(Core::i().fontItalic());
	auto displayName = displayName_;
	if(!keybind_.isSet())
		displayName += " [No keybind]";
	ImGui::Text(displayName.c_str());
	if(!isShownOption_.value() || !isActive())
		ImGui::PopFont();

	ImGui::PushFont(Core::i().fontIcon());
	
	int rv = 0;
	if(extremumIndicator != 1)
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - 2 * ImGui::GetFrameHeightWithSpacing());
		if(ImGui::Button((reinterpret_cast<const char*>(ICON_FA_ARROW_UP) + std::string("##PriorityValueUp") + nickname_).c_str()))
			rv = 1;
	}
	if(extremumIndicator != -1)
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - ImGui::GetFrameHeightWithSpacing());
		if(ImGui::Button((reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN) + std::string("##PriorityValueDown") + nickname_).c_str()))
			rv = -1;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor();

	return rv;
}

void WheelElement::SetShaderState()
{
	fVector4 adjustedColor = color();
	adjustedColor.x = Lerp(1, adjustedColor.x, colorizeAmount_);
	adjustedColor.y = Lerp(1, adjustedColor.y, colorizeAmount_);
	adjustedColor.z = Lerp(1, adjustedColor.z, colorizeAmount_);

	const float shadowOffsetMultiplier = -0.02f / 1024.f;

	fVector4 shadowData { shadowStrength_, shadowOffsetMultiplier * texWidth_, shadowOffsetMultiplier * texWidth_ * aspectRatio_, 1.f };
	
	auto fx = Core::i().mainEffect();
	
	{
		using namespace ShaderRegister::ShaderPS;
		using namespace ShaderRegister::ShaderVS;
        fx->SetVariable(ShaderType::PIXEL_SHADER, int_iElementID, elementId());
        fx->SetVariable(ShaderType::PIXEL_SHADER, float4_fElementColor, adjustedColor);
        fx->SetVariable(ShaderType::PIXEL_SHADER, float4_fShadowData, shadowData);
	    fx->SetVariable(ShaderType::PIXEL_SHADER, bool_bPremultiplyAlpha, premultiplyAlpha_);
	}
}

void WheelElement::Draw(int n, fVector4 spriteDimensions, size_t activeElementsCount, const mstime& currentTime, const WheelElement* elementHovered, const Wheel* parent)
{
	auto fx = Core::i().mainEffect();
	auto& quad = Core::i().quad();

	const float hoverTimer = hoverFadeIn(currentTime, parent);

	float elementAngle = float(n) / float(activeElementsCount) * 2 * float(M_PI);
	if (activeElementsCount == 1)
		elementAngle = 0;
	const fVector2 elementLocation = fVector2{cos(elementAngle - float(M_PI) / 2) * 0.2f, sin(elementAngle - float(M_PI) / 2) * 0.2f };

	spriteDimensions.x += elementLocation.x * spriteDimensions.z;
	spriteDimensions.y += elementLocation.y * spriteDimensions.w;

	float elementDiameter = float(sin((2 * M_PI / double(activeElementsCount)) / 2)) * 2.f * 0.2f * 0.66f;
	if (activeElementsCount == 1)
		elementDiameter = 2.f * 0.2f;
	else
		elementDiameter *= Lerp(1.f, 1.1f, SmoothStep(hoverTimer));

	switch (activeElementsCount)
	{
	case 1:
		spriteDimensions.z *= 0.5f;
		spriteDimensions.w *= 0.5f;
		break;
	case 2:
		spriteDimensions.z *= 0.7f;
		spriteDimensions.w *= 0.7f;
		break;
	case 3:
		spriteDimensions.z *= 0.9f;
		spriteDimensions.w *= 0.9f;
		break;
	case 4:
		spriteDimensions.z *= 0.95f;
		spriteDimensions.w *= 0.95f;
		break;
	default:
		break;
	}

	spriteDimensions.z *= elementDiameter;
	spriteDimensions.w *= elementDiameter;
	
	spriteDimensions.w *= aspectRatio_;

	SetShaderState();
	
	{
		using namespace ShaderRegister::ShaderPS;
		using namespace ShaderRegister::ShaderVS;
        fx->SetTexture(sampler2D_texMainSampler, appearance_.Get());
        fx->SetVariable(ShaderType::VERTEX_SHADER, float4_fSpriteDimensions, spriteDimensions);
	}
	
	fx->ApplyStates();
	quad->Draw();
}

float WheelElement::hoverFadeIn(const mstime& currentTime, const Wheel* parent) const
{
	const auto hoverIn = std::min(1.f, (currentTime - std::max(currentHoverTime_, parent->currentTriggerTime_ + parent->displayDelayOption_.value())) / 1000.f * 6);
	const auto hoverOut = 1.f - std::min(1.f, (currentTime - std::max(currentExitTime_, parent->currentTriggerTime_ + parent->displayDelayOption_.value())) / 1000.f * 6);

	return parent->currentHovered_ == this ? hoverIn : std::min(hoverIn, hoverOut);
}

}