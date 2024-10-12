#pragma once
#include "DynamicFormTracker.h"

struct Source {
    
    using SourceData = std::map<RefID,std::vector<StageInstance>>;
    using StageDict = std::map<StageNo, Stage>;

    SourceData data;  // change this to a map with refid as key and vector of instances as value

    FormID formid = 0;
    std::string editorid = "";
    DefaultSettings* defaultsettings = nullptr;  // eigentlich sollte settings heissen
    std::string qFormType;

    
    Source(const FormID id, const std::string id_str, 
        //RE::EffectSetting* e_m, 
        DefaultSettings* sttngs=nullptr)
        : formid(id), editorid(id_str), 
        //empty_mgeff(e_m), 
        defaultsettings(sttngs) { Init(); };

    [[maybe_unused]] [[nodiscard]] std::string_view GetName() const;

    RE::TESBoundObject* GetBoundObject() const { return GetFormByID<RE::TESBoundObject>(formid, editorid); };

    std::map<RefID,std::vector<StageUpdate>> UpdateAllStages(const std::vector<RefID>& filter, float time);

    // daha once yaratilmis bi stage olmasi gerekiyo
    bool IsStage(FormID some_formid);

    [[nodiscard]] inline bool IsStageNo(StageNo no) const;

    [[nodiscard]] inline bool IsFakeStage(StageNo no) const;

    // assumes that the formid exists as a stage!
    [[nodiscard]] StageNo GetStageNo(FormID formid_);

    const Stage& GetStage(StageNo no);
    [[nodiscard]] const Stage* GetStageSafe(StageNo no) const;

    [[nodiscard]] Duration GetStageDuration(StageNo no) const;

    [[nodiscard]] std::string GetStageName(StageNo no) const;

    StageInstance* InsertNewInstance(const StageInstance& stage_instance, RefID loc);

    StageInstance* InitInsertInstanceWO(StageNo n, Count c, RefID l, Duration t_0);

    // applies time modulation to all instances in the inventory
    [[nodiscard]] bool InitInsertInstanceInventory(StageNo n, Count c, RE::TESObjectREFR* inventory_owner,
                                                   Duration t_0);
    
    [[nodiscard]] bool MoveInstance(RefID from_ref, RefID to_ref, const StageInstance* st_inst);

    Count MoveInstances(RefID from_ref, RefID to_ref, FormID instance_formid, Count count, bool older_first);
    
    [[nodiscard]] inline bool IsTimeModulator(FormID _form_id) const;

    [[nodiscard]] bool IsDecayedItem(FormID _form_id) const;

    FormID inline GetModulatorInInventory(RE::TESObjectREFR* inventory_owner) const;

    inline FormID GetTransformerInInventory(RE::TESObjectREFR* inventory_owner) const;

    // always update before doing this
    void UpdateTimeModulationInInventory(RE::TESObjectREFR* inventory_owner, float _time);

    float GetNextUpdateTime(StageInstance* st_inst);

    void CleanUpData();

    void PrintData();

    void Reset();
    
    [[nodiscard]] bool IsHealthy() const { 
        return !init_failed;
	}

	const Stage& GetDecayedStage() const { return decayed_stage; }

private:

    void Init();

    RE::FormType formtype;
    std::set<StageNo> fake_stages;
    Stage decayed_stage;
    std::map<FormID, Stage> transformed_stages;

    std::vector<StageInstance*> queued_time_modulator_updates;
    bool init_failed = false;

    StageDict stages;

    // counta karismiyor
    [[nodiscard]] bool UpdateStageInstance(StageInstance& st_inst, float curr_time);

    template <typename T>
    void ApplyMGEFFSettings(T* stage_form, std::vector<StageEffect>& settings_effs) {
        RE::BSTArray<RE::Effect*> _effects = FormTraits<T>::GetEffects(stage_form);
        std::vector<FormID> MGEFFs;
        std::vector<uint32_t> pMGEFFdurations;
        std::vector<float> pMGEFFmagnitudes;

        // i need this many empty effects
        int n_empties = static_cast<int>(_effects.size()) - static_cast<int>(settings_effs.size());
        if (n_empties < 0) n_empties = 0;

        for (int j = 0; j < settings_effs.size(); j++) {
            MGEFFs.push_back(settings_effs[j].beffect);
            pMGEFFdurations.push_back(settings_effs.at(j).duration);
            pMGEFFmagnitudes.push_back(settings_effs.at(j).magnitude);
        }

        for (int j = 0; j < n_empties; j++) {
            MGEFFs.push_back(0);
            pMGEFFdurations.push_back(0);
            pMGEFFmagnitudes.push_back(0);
        }
        OverrideMGEFFs(_effects, MGEFFs, pMGEFFdurations, pMGEFFmagnitudes);
        
    }

    template <typename T>
    void GatherStages()  {
        for (StageNo stage_no: defaultsettings->numbers) {
            const auto stage_formid = defaultsettings->items[stage_no];
            if (!stage_formid && stage_no != 0) {
                if (Vector::HasElement(Settings::fakes_allowedQFORMS, qFormType))
                {
                    fake_stages.insert(stage_no);
                    continue;
                }
                else {
                    logger::critical("No ID given and copy items not allowed for this type {}", qFormType);
					return;
                }
            }

            if (stage_no == 0) RegisterStage(formid, stage_no);
			else {
                const auto stage_form = GetFormByID(stage_formid, "");
				if (!stage_form) {
                    logger::error("Stage form {} not found.", stage_formid);
					continue;
				}
                RegisterStage(stage_formid, stage_no);
			}
        }
    }

    size_t GetNStages() const;
    
    [[nodiscard]] Stage GetFinalStage() const;

    [[nodiscard]] Stage GetTransformedStage(FormID key_formid) const;

    void SetDelayOfInstances(float some_time, RE::TESObjectREFR* inventory_owner);

    void SetDelayOfInstance(StageInstance& instance, float curr_time,
                             RE::TESObjectREFR* inventory_owner) const;
   
    [[nodiscard]] bool CheckIntegrity();

    float GetDecayTime(const StageInstance& st_inst);

    inline void InitFailed();

    void RegisterStage(FormID stage_formid, StageNo stage_no);

    template <typename T>
    const FormID FetchFake(const StageNo st_no) {
        auto* DFT = DynamicFormTracker::GetSingleton();
        if (editorid.empty()) {
		    logger::error("Editorid is empty.");
		    return 0;
	    }
        const FormID new_formid = DFT->FetchCreate<T>(formid, editorid, static_cast<uint32_t>(st_no));

        if (const auto stage_form = GetFormByID<T>(new_formid)) {
            RegisterStage(new_formid, st_no);
            if (auto it = stages.find(st_no); it == stages.end()) {
                logger::error("Stage {} not found in stages.", st_no);
                DFT->Delete(new_formid);
                return 0;
            }

            // Update name of the fake form
            if (!stages.contains(st_no)) {
			    logger::error("Stage {} does not exist.", st_no);
			    DFT->Delete(new_formid);
			    return 0;
		    }
            const auto& name = stages.at(st_no).name;
            const auto og_name = RE::TESForm::LookupByID(formid)->GetName();
            const auto new_name = std::string(og_name) + " (" + name + ")";
            if (!name.empty() && std::strcmp(stage_form->fullName.c_str(), new_name.c_str()) != 0) {
                stage_form->fullName = new_name;
                logger::trace("Updated name of fake form to {}", name);
            }

            // Update value of the fake form
            const auto temp_value = defaultsettings->costoverrides[st_no];
            if (temp_value >= 0) FormTraits<T>::SetValue(stage_form, temp_value);
            // Update weight of the fake form
            const auto temp_weight = defaultsettings->weightoverrides[st_no];
            if (temp_weight >= 0) FormTraits<T>::SetWeight(stage_form, temp_weight);

            if (!defaultsettings->effects[st_no].empty() &&
                Vector::HasElement<std::string>(Settings::mgeffs_allowedQFORMS, qFormType)) {
                // change mgeff of fake form
                ApplyMGEFFSettings(stage_form, defaultsettings->effects[st_no]);
            }

        } else {
		    logger::error("Could not create copy form for source {}", editorid);
            DFT->Delete(new_formid);
		    return 0;
	    }
     
        return new_formid;
    };

    // also registers to stages
    FormID FetchFake(StageNo st_no);

    StageNo GetLastStageNo();
};
