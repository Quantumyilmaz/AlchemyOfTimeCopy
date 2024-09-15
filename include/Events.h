#pragma once
#include "Manager.h"

// MP <3
class OurEventSink : public RE::BSTEventSink<RE::TESEquipEvent>,
                     public RE::BSTEventSink<RE::TESActivateEvent>,
                     public RE::BSTEventSink<SKSE::CrosshairRefEvent>,
                     public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
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

    bool listen_menu = true;
    bool listen_cellchange = true;

    FormID consume_equipped_id;  // set in equip event only when equipped and used in container event (consume)
    float consume_equipped_t;
    RefID picked_up_refid;
    float picked_up_time;
    bool activate_eat = false;
    bool furniture_entered = false;

    bool world_objects_evolve = false;

    RE::NiPointer<RE::TESObjectREFR> furniture = nullptr;

    std::mutex mutex;

    void setListenMenu(bool val) { 
        std::lock_guard<std::mutex> lock(mutex);
        listen_menu = val; 
    }

    void setListenCellChange(bool val) { 
		std::lock_guard<std::mutex> lock(mutex);
		listen_cellchange = val; 
	}

    [[nodiscard]] bool getListenCellChange() {
        std::lock_guard<std::mutex> lock(mutex);
        return listen_cellchange;
    }

    [[nodiscard]] bool getListenMenu() { 
		std::lock_guard<std::mutex> lock(mutex);
		return listen_menu; 
	}

    void RefreshMenu(const RE::BSFixedString& menuName, RE::TESObjectREFR* inventory);

    void HandleWO(RE::TESObjectREFR* ref) const;


public:
    
    OurEventSink(bool wo_evolve, Manager* mngr)
        : world_objects_evolve(wo_evolve), M(mngr){};

    static OurEventSink* GetSingleton(bool wo_evolve, Manager* manager) {
        logger::info("Eventsink: worldobjectsevolve {}",wo_evolve);
        static OurEventSink singleton(wo_evolve,manager);
        return &singleton;
    }

    std::atomic<bool> block_eventsinks = false;


    void HandleWOsInCell();

    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*);

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event,
                                          RE::BSTEventSource<RE::TESActivateEvent>*);

    // to disable ref activation and external container-fake container placement
    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event,
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>*);
    
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*);
    
    // TAMAM GIBI
    RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* event,
                                          RE::BSTEventSource<RE::TESFurnitureEvent>*);

    // TAMAM
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,
                                                                   RE::BSTEventSource<RE::TESContainerChangedEvent>*);
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESSleepStopEvent*,
                                          RE::BSTEventSource<RE::TESSleepStopEvent>*);

    RE::BSEventNotifyControl ProcessEvent(const RE::TESWaitStopEvent*,
                                          RE::BSTEventSource<RE::TESWaitStopEvent>*);

    // https:  // github.com/SeaSparrowOG/RainExtinguishesFires/blob/c1aee0045aeb987b2f70e495b301c3ae8bd7b3a3/src/loadEventManager.cpp#L15
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*);

    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event,
                                          RE::BSTEventSource<RE::TESFormDeleteEvent>*);
};
