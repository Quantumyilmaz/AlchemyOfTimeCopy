#include "Events.h"

Manager* M = nullptr;
OurEventSink* eventSink = nullptr;

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        
        // 1) Check Po3's Tweaks
        if (!IsPo3Installed()) {
            logger::error("Po3 is not installed.");
            MsgBoxesNotifs::Windows::Po3ErrMsg();
            return;
        }

		// 2) Load settings
        LoadSettings();
        if (Settings::failed_to_load) {
            MsgBoxesNotifs::InGame::CustomMsg("Failed to load settings. Check log for details.");
            M->Uninstall();
			return;
        }

		// 3) Initialize Manager
        auto sources = std::vector<Source>();
        M = Manager::GetSingleton(sources);
        if (!M) return;
        
        // 4) Register event sinks
        bool wo_e = Settings::INI_settings["Other Settings"]["WorldObjectsEvolve"];
        eventSink = OurEventSink::GetSingleton(wo_e,M);
        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESActivateEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESContainerChangedEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESFurnitureEvent>(eventSink);
        RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESSleepStopEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESWaitStopEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESFormDeleteEvent>(eventSink);
        SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);
        RE::PlayerCharacter::GetSingleton()->AsBGSActorCellEventSource()->AddEventSink(eventSink);
        logger::info("Event sinks added.");
        
        // 5) Start MCP
        //UI::Register(M);
        logger::info("MCP registered.");
    }
    if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        if (!eventSink) return;
        eventSink->HandleWOsInCell();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    return true;
}