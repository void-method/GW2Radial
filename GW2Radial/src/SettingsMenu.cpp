#include <SettingsMenu.h>
#include <imgui.h>
#include <ImGuiExtensions.h>
#include <imgui_internal.h>
#include <UpdateCheck.h>

namespace GW2Radial
{

SettingsMenu::SettingsMenu()
	: showKeybind_("show_settings", "Show settings", "__core__", { GetScanCodeFromVirtualKey('M'), Modifier::SHIFT | Modifier::ALT }, false)
{
	showKeybind_.callback([&](Activated a) {
		if (a) {
			isVisible_ = true;
			Input::i().ClearActive();
		}
		return true;
	});
}

void SettingsMenu::OnInputLanguageChange()
{
	showKeybind_.key(GetScanCodeFromVirtualKey('M'));
}

void SettingsMenu::Draw()
{
	isFocused_ = false;

	if(isVisible_)
		Input::i().BlockKeybinds(1);
	else
		Input::i().UnblockKeybinds(1);

	if (isVisible_)
	{
		ImGui::SetNextWindowSize({ 750, 600 }, ImGuiCond_FirstUseEver);
		if(!ImGui::Begin("GW2Radial Options Menu", &isVisible_))
		{
			ImGui::End();
			return;
		}

		isFocused_ = ImGui::IsWindowFocused();
	
		if (!implementers_.empty())
		{
			if(currentTab_ == nullptr)
				currentTab_ = implementers_.front();

			if(ImGui::BeginTabBar("GW2RadialMainTabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
			    for (cref i : implementers_)
			    {
					if(!i->visible())
						continue;

				    if(ImGui::BeginTabItem(i->GetTabName(), nullptr, 0)) {
					    currentTab_ = i;
					    i->DrawMenu(&currentEditedKeybind_);
				        ImGui::EndTabItem();
				    }
			    }
				
			    ImGui::EndTabBar();
			}
		}

		ImGui::End();
	}
}

#if 0
void SettingsMenu::OnInputChange(bool changed, const std::set<ScanCode>& scs, const std::list<EventKey>& changedKeys, InputResponse& response)
{
	bool allowedThrough = false;
	if (allowThroughAlt_ != ScanCode::NONE && std::any_of(changedKeys.begin(), changedKeys.end(), [&](const auto& ek) { return ek.sc == allowThroughAlt_ && ek.down == false; })) {
		allowThroughAlt_ = ScanCode::NONE;
		allowedThrough = true;
	}

	if (allowThroughShift_ != ScanCode::NONE && std::any_of(changedKeys.begin(), changedKeys.end(), [&](const auto& ek) { return ek.sc == allowThroughShift_ && ek.down == false; })) {
		allowThroughShift_ = ScanCode::NONE;
		allowedThrough = true;
	}

	if (allowedThrough)
		return;


	if (isFocused_)
		response |= InputResponse::PREVENT_KEYBOARD;

	const bool isMenuKeybind = showKeybind_.matchesNoLeftRight(scs);
	if (isMenuKeybind) {
		if (isVisible_) {
			isVisible_ = false;
		}
		else
		{
		    isVisible_ = true;
		    Keybind::ForceRefreshDisplayStrings();
		}

		allowThroughAlt_ = scs.contains(ScanCode::ALTLEFT) ? ScanCode::ALTLEFT : ScanCode::ALTRIGHT;
		allowThroughShift_ = scs.contains(ScanCode::SHIFTLEFT) ? ScanCode::SHIFTLEFT : ScanCode::SHIFTRIGHT;

		response |= InputResponse::PREVENT_KEYBOARD;
	}
}
#endif

}