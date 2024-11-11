#pragma once
#include "CustomObjects.h"
#include <yaml-cpp/yaml.h>



namespace Settings {

    constexpr std::uint32_t kSerializationVersion = 627;
    constexpr std::uint32_t kDataKey = 'QAOT';
    constexpr std::uint32_t kDFDataKey = 'DAOT';
    
    inline bool failed_to_load = false;
    constexpr auto INI_path = L"Data/SKSE/Plugins/AlchemyOfTime.ini";
    const std::map<const char*, bool> moduleskeyvals = {{"FOOD",false},
														{"INGR",false},
                                                        {"MEDC",false},
                                                        {"POSN",false},
                                                        {"ARMO",false},
														{"WEAP",false},
														{"SCRL",false},
														{"BOOK",false},
														{"SLGM",false},
														{"MISC",false}
                                                        };
    const std::map<const char*, bool> otherkeysvals = {{"WorldObjectsEvolve", false}, {"bReset", false}, {"DisableWarnings",false}};
    const std::map<const char*, std::map<const char*, bool>> InISections = 
                   {{"Modules", moduleskeyvals}, {"Other Settings", otherkeysvals}};
    inline int nMaxInstances = 200000;
    inline int nForgettingTime = 2160;  // in hours
    inline bool disable_warnings = false;
    inline bool world_objects_evolve = false;



    const std::vector<std::string> fakes_allowedQFORMS = {"FOOD", "MISC"};
    const std::vector<std::string> xQFORMS = {"ARMO", "WEAP", "SLGM", "MEDC", "POSN"};  // xdata is carried over in item transitions
    const std::vector<std::string> mgeffs_allowedQFORMS = {"FOOD"};
    [[maybe_unused]] const std::vector<std::string> consumableQFORMS = {"FOOD", "INGR", "MEDC", "POSN", "SCRL", "BOOK", "SLGM", "MISC"};
    [[maybe_unused]] const std::vector<std::string> updateonequipQFORMS = {"ARMO", "WEAP"};
    const std::map<unsigned int, std::vector<std::string>> qform_bench_map = {
        {1, {"FOOD"}}
    };

    inline std::map<std::string,std::map<std::string, bool>> INI_settings;
    inline std::vector<std::string> QFORMS;
    inline std::map<std::string,DefaultSettings> defaultsettings;
    inline std::map<std::string, CustomSettings> custom_settings;
    inline std::map <std::string,std::vector<std::string>> exclude_list;

    [[nodiscard]] bool IsQFormType(FormID formid, const std::string& qformtype);

    inline std::string GetQFormType(FormID formid);

	[[nodiscard]] bool IsInExclude(FormID formid, std::string type = "");

	void AddToExclude(const std::string& entry_name, const std::string& type, const std::string& filename);

    [[nodiscard]] bool IsItem(FormID formid, std::string type = "", bool check_exclude = false);

    [[nodiscard]] bool IsItem(const RE::TESObjectREFR* ref, std::string type = "");


    // 0x99 - ExtraTextDisplayData 
    // 0x3C - ExtraSavedHavokData
    // 0x0B - ExtraPersistentCell
    // 0x48 - ExtraStartingWorldOrCell

    // 0x21 - ExtraOwnership
    // 0x24 - ExtraCount
    // 0x0E - ExtraStartingPosition //crahes when removed
    
    // 0x70 - ExtraEncounterZone 112
    // 0x7E - ExtraReservedMarkers 126
    // 0x88 - ExtraAliasInstanceArray 136
    // 0x8C - ExtraPromotedRef 140 NOT OK
    // 0x1C - ExtraReferenceHandle 28 NOT OK (npc muhabbeti)
    const std::vector<int> xRemove = {
        //0x99, 
        //0x3C, 0x0B, 0x48,
         //0x21, 
         // 
        //0x24,
                                //0x70, 0x7E, 0x88, 
        //0x8C, 0x1C
    };
};

std::vector<std::string> LoadExcludeList(const std::string& postfix);
DefaultSettings parseDefaults_(const YAML::Node& config);
DefaultSettings parseDefaults(std::string _type);
CustomSettings parseCustoms(const std::string& _type);
void LoadINISettings();
void LoadSettings();


namespace LogSettings {
#ifndef NDEBUG
    inline bool log_trace = true;
#else
	inline bool log_trace = false;
#endif
    inline bool log_info = true;
    inline bool log_warning = true;
    inline bool log_error = true;
};