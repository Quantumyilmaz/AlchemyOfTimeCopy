#include "Settings.h"
#include "SimpleIni.h"


bool Settings::IsQFormType(const FormID formid, const std::string& qformtype) {
    // POPULATE THIS
    const auto* form = GetFormByID(formid);
    if (qformtype == "FOOD") return IsFoodItem(form);
    if (qformtype == "INGR") return FormIsOfType(form, RE::IngredientItem::FORMTYPE);
    if (qformtype == "MEDC") return IsMedicineItem(form);
    if (qformtype == "POSN") return IsPoisonItem(form);
    if (qformtype == "ARMO") return FormIsOfType(form,RE::TESObjectARMO::FORMTYPE);
    if (qformtype == "WEAP") return FormIsOfType(form,RE::TESObjectWEAP::FORMTYPE);
    if (qformtype == "SCRL") return FormIsOfType(form, RE::ScrollItem::FORMTYPE);
	if (qformtype == "BOOK") return FormIsOfType(form,RE::TESObjectBOOK::FORMTYPE);
    if (qformtype == "SLGM") return FormIsOfType(form, RE::TESSoulGem::FORMTYPE);
	if (qformtype == "MISC") return FormIsOfType(form,RE::TESObjectMISC::FORMTYPE);
    return false;
}

std::string Settings::GetQFormType(const FormID formid)
{
    for (const auto& q_ftype : Settings::QFORMS) {
        if (Settings::IsQFormType(formid,q_ftype)) return q_ftype;
	}
	return "";
}

bool Settings::IsInExclude(const FormID formid, std::string type) {
    const auto form = GetFormByID(formid);
    if (!form) {
        logger::warn("Form not found.");
        return false;
    }
        
    if (type.empty()) type = GetQFormType(formid);
    if (type.empty()) {
        //logger::trace("Type is empty. for formid: {}", formid);
		return false;
	}
    if (!Settings::exclude_list.contains(type)) {
		logger::critical("Type not found in exclude list. for formid: {}", formid);
        return false;
    }

    std::string form_string = std::string(form->GetName());

    if (std::string form_editorid = clib_util::editorID::get_editorID(form); !form_editorid.empty() && String::includesWord(form_editorid, Settings::exclude_list[type])) {
		logger::trace("Form is in exclude list.form_editorid: {}", form_editorid);
		return true;
	}

    /*const auto exlude_list = LoadExcludeList(postfix);*/
    if (String::includesWord(form_string, Settings::exclude_list[type])) {
        logger::trace("Form is in exclude list.form_string: {}", form_string);
        return true;
    }
    return false;
}

void Settings::AddToExclude(const std::string& entry_name, const std::string& type, const std::string& filename)
{
	if (entry_name.empty() || type.empty() || filename.empty()) {
		logger::warn("Empty entry_name, type or filename.");
		return;
	}
	// check if type is a valid qformtype
	if (!std::ranges::any_of(Settings::QFORMS, [type](const std::string& qformtype) { return qformtype == type; })) {
		logger::warn("AddToExclude: Invalid type: {}", type);
		return;
	}

	const auto folder_path = "Data/SKSE/Plugins/AlchemyOfTime/" + type + "/exclude";
	std::filesystem::create_directories(folder_path);
	const auto file_path = folder_path + "/" + filename + ".txt";
	// check if the entry is already in the list
	if (std::ranges::find(Settings::exclude_list[type], entry_name) != Settings::exclude_list[type].end()) {
		logger::warn("Entry already in exclude list: {}", entry_name);
		return;
	}
	std::ofstream file(file_path, std::ios::app);
	file << entry_name << std::endl;
	file.close();
	Settings::exclude_list[type].push_back(entry_name);
}

bool Settings::IsItem(const FormID formid, std::string type, const bool check_exclude) {
    if (!formid) return false;
    if (check_exclude && Settings::IsInExclude(formid, type)) return false;
    if (type.empty()) return !GetQFormType(formid).empty();
	return IsQFormType(formid, type);
}

bool Settings::IsItem(const RE::TESObjectREFR* ref, std::string type) {
    if (!ref) return false;
    if (ref->IsDisabled()) return false;
    if (ref->IsDeleted()) return false;
    const auto base = ref->GetBaseObject();
    if (!base) return false;
    return IsItem(base->GetFormID(),type);
}

std::vector<std::string> LoadExcludeList(const std::string& postfix)
 {
    const auto folder_path = "Data/SKSE/Plugins/AlchemyOfTime/" + postfix + "/exclude";
    logger::trace("Exclude path: {}", folder_path);

    // Create folder if it doesn't exist
    std::filesystem::create_directories(folder_path);
    std::set<std::string> strings;

    // Iterate over files in the folder
    for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
        // Check if the entry is a regular file and ends with ".txt"
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) strings.insert(line);
            }
        }
    }

    auto result = Vector::SetToVector(strings);
    return result;
}

DefaultSettings parseDefaults_(const YAML::Node& config)
 {
    logger::trace("Parsing settings.");
    DefaultSettings settings;
        //we have:stages, decayedFormEditorID and delayers
    if (!config["stages"] || config["stages"].size() == 0) {
        logger::error("Stages are empty.");
        return settings;
    }
    for (const auto& stageNode : config["stages"]) {
        if (!stageNode["no"] || stageNode["no"].IsNull()) {
			logger::error("No is missing for stage.");
            return settings;
		}
        const auto temp_no = stageNode["no"].as<StageNo>();
        // add to numbers
        logger::trace("Stage no: {}", temp_no);
        settings.numbers.push_back(temp_no);
        const auto temp_formeditorid = stageNode["FormEditorID"] && !stageNode["FormEditorID"].IsNull()
                                            ? stageNode["FormEditorID"].as<std::string>()
                                            : "";
        const FormID temp_formid = temp_formeditorid.empty()
                                        ? 0
                                        : GetFormEditorIDFromString(temp_formeditorid);
        if (!temp_formid && !temp_formeditorid.empty()) {
            logger::error("Formid could not be obtained for {}", temp_formid, temp_formeditorid);
            return DefaultSettings();
        }
        // add to items
        logger::trace("Formid");
        settings.items[temp_no] = temp_formid;
        // add to durations
        logger::trace("Duration");
        settings.durations[temp_no] = stageNode["duration"].as<Duration>();
        // add to stage_names
        logger::trace("Name");
        if (!stageNode["name"].IsNull()) {
            const auto temp_name = stageNode["name"].as<StageName>();
            // if it is empty, or just whitespace, set it to empty
            if (temp_name.empty() || std::ranges::all_of(temp_name, isspace))
				settings.stage_names[temp_no] = "";
			else settings.stage_names[temp_no] = stageNode["name"].as<StageName>();
        } else settings.stage_names[temp_no] = "";
        // add to costoverrides
        logger::trace("Cost");
        if (stageNode["value"] && !stageNode["value"].IsNull()) {
            settings.costoverrides[temp_no] = stageNode["value"].as<int>();
        } else settings.costoverrides[temp_no] = -1;
        // add to weightoverrides
        logger::trace("Weight");
        if (stageNode["weight"] && !stageNode["weight"].IsNull()) {
            settings.weightoverrides[temp_no] = stageNode["weight"].as<float>();
        } else settings.weightoverrides[temp_no] = -1.0f;
            
        // add to crafting_allowed
        logger::trace("Crafting");
        if (stageNode["crafting_allowed"] && !stageNode["crafting_allowed"].IsNull()){
            settings.crafting_allowed[temp_no] = stageNode["crafting_allowed"].as<bool>();
        } else settings.crafting_allowed[temp_no] = false;


        // add to effects
        logger::trace("Effects");
        std::vector<StageEffect> effects;
        if (!stageNode["mgeffect"] || stageNode["mgeffect"].size() == 0) {
			logger::trace("Effects are empty. Skipping.");
        } else {
            for (const auto& effectNode : stageNode["mgeffect"]) {
                const auto temp_effect_formeditorid =
                    effectNode["FormEditorID"] && !effectNode["FormEditorID"].IsNull() ? effectNode["FormEditorID"].as<std::string>() : "";
                const FormID temp_effect_formid =
                    temp_effect_formeditorid.empty()
                        ? 0
                        : GetFormEditorIDFromString(temp_effect_formeditorid);
                if (temp_effect_formid>0){
                    const auto temp_magnitude = effectNode["magnitude"].as<float>();
                    const auto temp_duration = effectNode["duration"].as<DurationMGEFF>();
                    effects.emplace_back(temp_effect_formid, temp_magnitude, temp_duration);
                } else effects.emplace_back(temp_effect_formid, 0.f, 0);
                // currently only one allowed
                break;
            }
        }
        settings.effects[temp_no] = effects;
    }
    // final formid
    logger::trace("terminal item");
    const FormID temp_decayed_id =
        config["finalFormEditorID"] && !config["finalFormEditorID"].IsNull()
			? GetFormEditorIDFromString(config["finalFormEditorID"].as<std::string>())
			: 0;
    if (!temp_decayed_id) {
        logger::error("Decayed id is 0.");
        return DefaultSettings();
    }
    logger::trace("Decayed id: {}", temp_decayed_id);
    settings.decayed_id = temp_decayed_id;
    // delayers
    logger::trace("timeModulators");
    for (const auto& modulator : config["timeModulators"]) {
        const auto temp_formeditorid = modulator["FormEditorID"] && !modulator["FormEditorID"].IsNull()
                                            ? modulator["FormEditorID"].as<std::string>()
                                            : "";
        const FormID temp_formid = GetFormEditorIDFromString(temp_formeditorid);
        settings.delayers[temp_formid] = !modulator["magnitude"].IsNull() ? modulator["magnitude"].as<float>() : 1;
		settings.delayers_order.push_back(temp_formid);
    }

    for (const auto& transformer : config["transformers"]) {
        const auto temp_formeditorid = transformer["FormEditorID"] &&
                                        !transformer["FormEditorID"].IsNull() ? transformer["FormEditorID"].as<std::string>() : "";
        const FormID temp_formid = GetFormEditorIDFromString(temp_formeditorid);

        const auto temp_finalFormEditorID =
            transformer["finalFormEditorID"] &&
            !transformer["finalFormEditorID"].IsNull() ? transformer["finalFormEditorID"].as<std::string>() : "";
        const FormID temp_formid2 = GetFormEditorIDFromString(temp_finalFormEditorID);
        if (!transformer["duration"] || transformer["duration"].IsNull()) {
			logger::warn("Duration is missing.");
			continue;
		}
        const auto temp_duration = transformer["duration"].as<float>();
        std::vector<StageNo> allowed_stages;
        if (!transformer["allowed_stages"]) {
            // default is all stages
            for (const auto& key : settings.items | std::views::keys) {
                allowed_stages.push_back(key);
            }
        } 
        else if (transformer["allowed_stages"].IsScalar()) {
            allowed_stages.push_back(transformer["allowed_stages"].as<StageNo>());
        } 
        else allowed_stages = transformer["allowed_stages"].as<std::vector<StageNo>>();
        if (allowed_stages.empty()) {
            for (const auto& key : settings.items | std::views::keys) {
				allowed_stages.push_back(key);
			}
        }
        auto temp_tuple = std::make_tuple(temp_formid2, temp_duration, allowed_stages);
        settings.transformers[temp_formid] = temp_tuple;
		settings.transformers_order.push_back(temp_formid);
    }
        
    if (!settings.CheckIntegrity()) {
		logger::critical("Settings integrity check failed.");
	}

    return settings;
}


DefaultSettings parseDefaults(std::string _type)
{ 
    const auto filename = "Data/SKSE/Plugins/AlchemyOfTime/" + _type + "/AoT_default" + _type + ".yml";

	if (FileIsEmpty(filename)) {
		logger::info("File is empty: {}", filename);
		return {};
	}

    logger::info("Filename: {}", filename);
    const YAML::Node config = YAML::LoadFile(filename);
    logger::info("File loaded.");
    auto temp_settings = parseDefaults_(config);
    if (!temp_settings.CheckIntegrity()) {
        logger::warn("parseDefaults: Settings integrity check failed for {}", _type);
    }
    return temp_settings;
}
CustomSettings parseCustoms(const std::string& _type)
{
    CustomSettings _custom_settings;
    const auto folder_path = "Data/SKSE/Plugins/AlchemyOfTime/" + _type + "/custom";
    std::filesystem::create_directories(folder_path);
    logger::info("Custom path: {}", folder_path);
        
    for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {

        if (entry.is_regular_file() && entry.path().extension() == ".yml") {
            const auto filename = entry.path().string();

            if (FileIsEmpty(filename)) {
				logger::info("File is empty: {}", filename);
				continue;
            };

            YAML::Node config = YAML::LoadFile(filename);

            if (!config["ownerLists"]) {
				logger::warn("OwnerLists not found in {}", filename);
				continue;
			}

            for (const auto& Node_ : config["ownerLists"]){
				if (!Node_["owners"]) {
					logger::warn("Owners not found in {}", filename);
					continue;
				}
                // we have list of owners at each node or a scalar owner
                if (Node_["owners"].IsScalar()) {
                    const auto ownerName = Node_["owners"].as<std::string>();
                    if (auto temp_settings = parseDefaults_(Node_); temp_settings.CheckIntegrity())
                        _custom_settings[std::vector<std::string>{ownerName}] = temp_settings;
			    } 
                else {
				    std::vector<std::string> owners;
                    for (const auto& owner : Node_["owners"]) {
					    owners.push_back(owner.as<std::string>());
				    }

                    if (auto temp_settings = parseDefaults_(Node_); temp_settings.CheckIntegrity())
                        _custom_settings[owners] = temp_settings;
			    }
            }
        }
    }
    return _custom_settings;
}

void LoadINISettings()
{
    logger::info("Loading ini settings");


    CSimpleIniA ini;

    ini.SetUnicode();
    ini.LoadFile(Settings::INI_path);

    // if the section does not exist populate with default values
    for (const auto& [section, defaults] : Settings::InISections) {
        if (!ini.GetSection(section)) {
			for (const auto& [key, val] : defaults) {
				ini.SetBoolValue(section, key, val);
                Settings::INI_settings[section][key] = val;
			}
		}
        // it exist now check if we have values for all keys
        else {
			for (const auto& [key, val] : defaults) {
                if (const auto temp_bool = ini.GetBoolValue(section, key, val); temp_bool == val) {
					ini.SetBoolValue(section, key, val);
					Settings::INI_settings[section][key] = val;
				} else Settings::INI_settings[section][key] = temp_bool;
			}
        }
	}
    if (!ini.KeyExists("Other Settings", "nMaxInstancesInThousands")) {
        ini.SetLongValue("Other Settings", "nMaxInstancesInThousands", 200);
        Settings::nMaxInstances = 200000;
    } else Settings::nMaxInstances = 1000 * ini.GetLongValue("Other Settings", "nMaxInstancesInThousands", 200);

    Settings::nMaxInstances = std::min(Settings::nMaxInstances, 2000000);

    if (!ini.KeyExists("Other Settings", "nTimeToForgetInDays")) {
        ini.SetLongValue("Other Settings", "nTimeToForgetInDays", 90);
        Settings::nForgettingTime = 2160;
    } else Settings::nForgettingTime = 24 * ini.GetLongValue("Other Settings", "nTimeToForgetInDays", 90);

    Settings::nForgettingTime = std::min(Settings::nForgettingTime, 4320);

    Settings::disable_warnings = ini.GetBoolValue("Other Settings", "DisableWarnings", Settings::disable_warnings);
    Settings::world_objects_evolve = ini.GetBoolValue("Other Settings", "WorldObjectsEvolve", Settings::world_objects_evolve);
		
    ini.SaveFile(Settings::INI_path);
}

void LoadSettings()
{
    logger::info("Loading settings.");
    try {
        LoadINISettings();
    } catch (const std::exception& ex) {
		logger::critical("Failed to load ini settings: {}", ex.what());
		Settings::failed_to_load = true;
		return;
	}
    if (!Settings::INI_settings.contains("Modules")) {
        logger::critical("Modules section not found in ini settings.");
		Settings::failed_to_load = true;
		return;
	}
    for (const auto& [key,val]: Settings::INI_settings["Modules"]) {
        if (val) Settings::QFORMS.push_back(key);
	}
    for (const auto& _qftype: Settings::QFORMS) {
        try {
            logger::info("Loading Settings::defaultsettings for {}", _qftype);
			if (auto temp_default_settings = parseDefaults(_qftype); !temp_default_settings.IsEmpty()) Settings::defaultsettings[_qftype] = temp_default_settings;
        } catch (const std::exception& ex) {
            logger::critical("Failed to load default settings for {}: {}", _qftype, ex.what());
            Settings::failed_to_load = true;
            return;
        }
        try {
            logger::info("Loading custom settings for {}", _qftype);
			if (auto temp_custom_settings = parseCustoms(_qftype); !temp_custom_settings.empty()) Settings::custom_settings[_qftype] = temp_custom_settings;
        } catch (const std::exception& ex) {
			logger::critical("Failed to load custom settings for {}: {}", _qftype, ex.what());
			Settings::failed_to_load = true;
            return;
        }
        for (const auto& key : Settings::custom_settings[_qftype] | std::views::keys) {
            logger::trace("Key: {}", key.front());
        }
        try {
            Settings::exclude_list[_qftype] = LoadExcludeList(_qftype);
        } catch (const std::exception& ex) {
			logger::critical("Failed to load exclude list for {}: {}", _qftype, ex.what());
            Settings::failed_to_load = true;
            return;
        }
	}
}
