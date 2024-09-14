#include "Events.h"

Manager* M = nullptr;
bool eventsinks_added = false;

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        logger::info("Data loaded.");
        // Start
        if (!IsPo3Installed()) {
            logger::error("Po3 is not installed.");
            MsgBoxesNotifs::Windows::Po3ErrMsg();
            return;
        }
        auto DFT = DynamicFormTracker::GetSingleton();
        LoadSettings();
        auto sources = std::vector<Source>();
        M = Manager::GetSingleton(sources);
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type ==
        SKSE::MessagingInterface::kPostLoadGame) {
        if (eventsinks_added) return;
        logger::info("New or Post-load game.");
        if (Settings::failed_to_load) {
            MsgBoxesNotifs::InGame::CustomMsg("Failed to load settings. Check log for details.");
            M->Uninstall();
			return;
        }
        // Post-load
        if (!M) return;
        // EventSink
        bool wo_e = Settings::INI_settings["Other Settings"]["WorldObjectsEvolve"];
        auto* eventSink = OurEventSink::GetSingleton(wo_e,M);
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
        eventsinks_added = true;
        logger::info("Event sinks added.");
        
        if (message->type != SKSE::MessagingInterface::kNewGame) {
            eventSink->HandleWOsInCell();
        }
        
        // MCP
        //UI::Register(M);
        //logger::info("MCP registered.");
    }
}

static void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
#endif
    logger::info("Name of the plugin is {}.", pluginName);
    logger::info("Version of the plugin is {}.", SKSE::PluginDeclaration::GetSingleton()->GetVersion());
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    return true;
}