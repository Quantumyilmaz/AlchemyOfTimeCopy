#pragma once
#include "CustomObjects.h"

namespace Settings {

    inline int nMaxInstances = 200000;

    inline std::map<std::string,std::map<std::string, bool>> INI_settings;
    
    using CustomSettings = std::map<std::vector<std::string>, DefaultSettings>;

    const std::vector<std::string> fakes_allowedQFORMS = {"FOOD", "MISC"};
    const std::vector<std::string> xQFORMS = {"ARMO", "WEAP", "SLGM", "MEDC", "POSN"};  // xdata is carried over in item transitions
    const std::vector<std::string> mgeffs_allowedQFORMS = {"FOOD"};

    inline int nForgettingTime = 2160;  // in hours

    inline std::vector<std::string> QFORMS;
    inline std::map<std::string,DefaultSettings> defaultsettings;
    inline std::map<std::string, CustomSettings> custom_settings;
    inline std::map <std::string,std::vector<std::string>> exclude_list;

    [[nodiscard]] const bool IsQFormType(const FormID formid, const std::string& qformtype);

    inline std::string GetQFormType(const FormID formid);

	[[nodiscard]] const bool IsInExclude(const FormID formid, std::string type = "");

    [[nodiscard]] const bool IsItem(const FormID formid, std::string type = "", bool check_exclude=false);

    [[nodiscard]] const bool IsItem(const RE::TESObjectREFR* ref, std::string type = "");
};