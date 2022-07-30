#include "Hooks.h"

#include "Events.h"
#include "Settings.h"

namespace Hooks
{
	void Install()
	{
		logger::trace("Hooking...");

		SprintHandlerHook::Hook();

		logger::trace("...success");
	}

	static bool bStoppingSprint = false;

	void SprintHandlerHook::ProcessButton(RE::SprintHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		using FlagBDD = RE::PlayerCharacter::FlagBDD;

		if (Settings::bUseSprintButton && !REL::Module::IsVR()) {
			auto playerCharacter = RE::PlayerCharacter::GetSingleton();

			if (playerCharacter) {
                // unkBDD is the sprint control bit
                // Only aviable in AE/SE, but not in VR yet
                auto& unkBDD = REL::RelocateMember<RE::stl::enumeration<FlagBDD, std::uint8_t>>(playerCharacter, 0xBDD, 0x0);
                auto playerSprinting = [&]() { return (unkBDD & FlagBDD::kSprinting) != FlagBDD::kNone; };

				if (a_event->IsDown() && playerSprinting()) {  // stopping sprint
					bStoppingSprint = true;
				} else 
				if (a_event->HeldDuration() < Settings::fSprintHoldDuration) {
					if (a_event->IsUp())
					{
						Events::Dodge();
						bStoppingSprint = false;
					}
					return;
                } else if (!playerSprinting() && !bStoppingSprint) {
					a_event->heldDownSecs = 0.f;
				} else if (a_event->IsUp()) {
					bStoppingSprint = false;
				}
			}
		}
				
		_ProcessButton(a_this, a_event, a_data);
	}
}
