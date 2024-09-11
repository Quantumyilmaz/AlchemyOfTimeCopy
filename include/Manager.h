#pragma once
#include "Data.h"
#include "Ticker.h"

class Manager : public Ticker, public SaveLoadData {
	RE::TESObjectREFR* player_ref = RE::PlayerCharacter::GetSingleton()->As<RE::TESObjectREFR>();
	
    // std::map<RefID,std::set<FormID>> external_favs;
    
    std::map<Types::FormFormID, std::pair<int, Count>>
        handle_crafting_instances;  // formid1: source formid, formid2: stage formid
    std::map<FormID, bool> faves_list;
    std::map<FormID, bool> equipped_list;

    std::map<RefID, std::vector<FormID>> locs_to_be_handled;  // onceki sessiondan kalan fake formlar

    bool worldobjectsevolve = false;
    bool should_reset = false;

    // Use Or Take Compatibility
    bool po3_use_or_take = false;

    // 0x0003eb42 damage health

    bool listen_activate = true;
    bool listen_equip = true;
    bool listen_crosshair = true;
    bool listen_container_change = true;
    bool listen_menuopenclose = true;

    bool isUninstalled = false;

    std::mutex mutex;

    std::vector<Source> sources;

    std::unordered_map<std::string, bool> _other_settings;

    unsigned int _instance_limit = 200000;

    std::map<RefID, float> _ref_stops_;
    bool listen_woupdate = true;
    bool update_is_busy = false;

    std::vector<std::tuple<FormID, Count, RefID, Duration>> to_register_go;

    std::set<FormID> do_not_register;

    void _WOUpdateLoop(const float curr_time);

    void UpdateLoop();

    void QueueWOUpdate(RefID refid, float stop_t);

    void RemoveFromWOUpdateQueue(RefID refid);

    [[nodiscard]] const unsigned int GetNInstances();

    [[nodiscard]] Source* _MakeSource(const FormID source_formid, DefaultSettings* settings);

    void CleanUpSourceData(Source* src);

    [[nodiscard]] Source* GetSource(const FormID some_formid);

    [[nodiscard]] Source* ForceGetSource(const FormID some_formid);

    [[nodiscard]] StageInstance* GetWOStageInstance(RefID wo_refid);

    [[nodiscard]] StageInstance* GetWOStageInstance(RE::TESObjectREFR* wo_ref);

    [[nodiscard]] Source* GetWOSource(RefID wo_refid);

    [[nodiscard]] Source* GetWOSource(RE::TESObjectREFR* wo_ref);

    inline void _ApplyStageInWorld_Fake(RE::TESObjectREFR* wo_ref, const char* xname);

    inline void _ApplyStageInWorld_Custom(RE::TESObjectREFR* wo_ref, RE::TESBoundObject* stage_bound);

    void ApplyStageInWorld(RE::TESObjectREFR* wo_ref, const Stage& stage, RE::TESBoundObject* source_bound = nullptr);

    inline void _ApplyEvolutionInInventoryX(RE::TESObjectREFR* inventory_owner, Count update_count, FormID old_item,
                                     FormID new_item);

    inline void _ApplyEvolutionInInventory(RE::TESObjectREFR* inventory_owner, Count update_count, FormID old_item,
                                    FormID new_item);

    void ApplyEvolutionInInventory(std::string _qformtype_, RE::TESObjectREFR* inventory_owner, Count update_count,
                                   FormID old_item, FormID new_item);

    inline const RE::ObjectRefHandle RemoveItemReverse(RE::TESObjectREFR* moveFrom, RE::TESObjectREFR* moveTo, FormID item_id,
                                                Count count, RE::ITEM_REMOVE_REASON reason);

    void AddItem(RE::TESObjectREFR* addTo, RE::TESObjectREFR* addFrom, FormID item_id, Count count);

    bool _UpdateStagesInSource(std::vector<RE::TESObjectREFR*> refs, Source& src, const float curr_time);

    bool _UpdateStagesOfRef(RE::TESObjectREFR* ref, const float _time, bool is_inventory);
    
    // only for time modulators which are also stages.
    bool _UpdateTimeModulators(RE::TESObjectREFR* inventory_owner, const float curr_time);

    void RaiseMngrErr(const std::string err_msg_ = "Error");

    void InitFailed();

    void Init();

    void setListenEquip(const bool value);

    void setListenWOUpdate(const bool value);

    void setListenContainerChange(const bool value);

    [[nodiscard]] const bool getListenWOUpdate();

    void setUpdateIsBusy(const bool);

    [[nodiscard]] const bool getUpdateIsBusy();

public:
    Manager(std::vector<Source>& data, std::chrono::milliseconds interval)
        : sources(data), Ticker([this]() { UpdateLoop(); }, interval) {
        Init();
    };

    static Manager* GetSingleton(std::vector<Source>& data, int u_intervall = 3000) {
        static Manager singleton(data, std::chrono::milliseconds(u_intervall));
        return &singleton;
    }

	const char* GetType() override { return "Manager"; }

    inline void Uninstall() {
        isUninstalled = true;
        // Uninstall other settings...
        // Settings::UninstallOtherSettings();
    }

    // use it only for world objects! checks if there is a stage instance for the given refid
    [[nodiscard]] const bool RefIsRegistered(const RefID refid);

    [[nodiscard]] const bool RegisterAndGo(const FormID some_formid, const Count count, const RefID location_refid,
                                           Duration register_time = 0);

    // giris noktasi RegisterAndGo uzerinden
    [[nodiscard]] const bool RegisterAndGo(RE::TESObjectREFR* wo_ref);

    bool UpdateStages(RE::TESObjectREFR* ref, const float _time = 0);

    bool UpdateStages(RefID loc_refid);

    // for syncing the previous session's data with the current session
    void _HandleLoc(RE::TESObjectREFR* loc_ref);

    void Print();

};