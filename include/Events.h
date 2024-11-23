#pragma once
#include "Hooks.h"

// MP <3
class OurEventSink final : public RE::BSTEventSink<RE::TESEquipEvent>,
                           public RE::BSTEventSink<RE::TESActivateEvent>,
                           public RE::BSTEventSink<SKSE::CrosshairRefEvent>,
                           public RE::BSTEventSink<RE::TESFurnitureEvent>,
                           public RE::BSTEventSink<RE::TESContainerChangedEvent>,
                           public RE::BSTEventSink<RE::TESSleepStopEvent>,
                           public RE::BSTEventSink<RE::TESWaitStopEvent>,
                           public RE::BSTEventSink<RE::BGSActorCellEvent>,
                           public RE::BSTEventSink<RE::TESFormDeleteEvent> {

    OurEventSink() = default;
    OurEventSink(const OurEventSink&) = delete;
    OurEventSink(OurEventSink&&) = delete;
    OurEventSink& operator=(const OurEventSink&) = delete;
    OurEventSink& operator=(OurEventSink&&) = delete;


    RE::UI* ui = RE::UI::GetSingleton();
    Manager* M = nullptr;

    std::atomic<bool> listen_cellchange = true;

    FormID consume_equipped_id;  // set in equip event only when equipped and used in container event (consume)
    RefID picked_up_refid;

    bool furniture_entered = false;
    RE::NiPointer<RE::TESObjectREFR> furniture = nullptr;

	/*const float min_last_crosshair_update_time = 0.0003f;
	std::pair<RefID, float> last_crosshair_ref_update = { 0,0.f };*/

    void HandleWO(RE::TESObjectREFR* ref) const;

public:
    
    OurEventSink(Manager* mngr)
        :  M(mngr){}

    static OurEventSink* GetSingleton(Manager* manager) {
        static OurEventSink singleton(manager);
        return &singleton;
    }

    std::atomic<bool> block_eventsinks = false;


    void HandleWOsInCell() const;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) override;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event,
                                          RE::BSTEventSource<RE::TESActivateEvent>*) override;

    // to disable ref activation and external container-fake container placement
    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event,
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override;
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* event,
                                          RE::BSTEventSource<RE::TESFurnitureEvent>*) override;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,
                                                                   RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESSleepStopEvent*,
                                          RE::BSTEventSource<RE::TESSleepStopEvent>*) override;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESWaitStopEvent*,
                                          RE::BSTEventSource<RE::TESWaitStopEvent>*) override;

    // https://github.com/SeaSparrowOG/RainExtinguishesFires/blob/c1aee0045aeb987b2f70e495b301c3ae8bd7b3a3/src/loadEventManager.cpp#L15
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event,
                                          RE::BSTEventSource<RE::TESFormDeleteEvent>*) override;
};
