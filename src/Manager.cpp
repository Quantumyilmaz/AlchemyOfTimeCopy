#include "Manager.h"

void Manager::_WOUpdateLoop(const float curr_time)
{
	std::map<RefID, float> ref_stops_copy = _ref_stops_;
    std::vector<std::pair<RefID, float>> ref_n_stops;
    for (auto& [refid, stop_t] : ref_stops_copy) {
        if (stop_t <= curr_time) ref_n_stops.push_back({refid, stop_t});
    }

    if (ref_n_stops.empty()) {
        Stop();
        return;
    }

    std::sort(ref_n_stops.begin(), ref_n_stops.end(),
                [](std::pair<RefID, float> a, std::pair<RefID, float> b) { return a.second < b.second; });

    auto stop_t = ref_n_stops[0].second;
    if (stop_t > curr_time) return;
    const auto refid = ref_n_stops[0].first;
    if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(refid)) {
        logger::trace("_WOUpdateLoop: Queued Update for {}.", ref->GetName());
        if (!UpdateStages(ref, curr_time)) {
            logger::warn("Queued Update failed for {}. update_time {}, curr_time {}", refid, stop_t, curr_time);
            _ref_stops_.erase(refid);
        } else if (_ref_stops_.contains(refid) && _ref_stops_.at(refid) == stop_t) {
            _ref_stops_.erase(refid);
        }
        _WOUpdateLoop(curr_time);
    }
}

void Manager::UpdateLoop()
{
    if (!getListenWOUpdate()) return;
    setListenWOUpdate(false);
    if (_ref_stops_.empty()) {
        Stop();
        return setListenWOUpdate(true);
    }
    logger::trace("UpdateLoop");
    if (auto ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) return setListenWOUpdate(true);
    if (auto cal = RE::Calendar::GetSingleton()) {
        const auto curr_time = cal->GetHoursPassed();
        logger::trace("_WOUpdateLoop");
        _WOUpdateLoop(curr_time);
    }
    logger::trace("UpdateLoop done.");
    setListenWOUpdate(true);
    Start();
}

void Manager::QueueWOUpdate(RefID refid, float stop_t)
{
    if (!worldobjectsevolve) return;
    _ref_stops_[refid] = stop_t;
    Start();
    logger::trace("Queued WO update for {} with stop time {}", refid, stop_t);
}

void Manager::RemoveFromWOUpdateQueue(RefID refid)
{
    if (_ref_stops_.contains(refid)) {
        _ref_stops_.erase(refid);
        logger::trace("Removed from WO update queue: {}", refid);
    }
}

const unsigned int Manager::GetNInstances()
{
    unsigned int n = 0;
    for (auto& src : sources) {
        for (const auto& [loc, _] : src.data) {
            n += static_cast<unsigned int>(src.data[loc].size());
        }
    }
    return n;
}

Source* Manager::_MakeSource(const FormID source_formid, DefaultSettings* settings)
{
    if (!source_formid) return nullptr;
    if (IsDynamicFormID(source_formid)) return nullptr;
    // Source new_source(source_formid, "", empty_mgeff, settings);
    Source new_source(source_formid, "", settings);
    if (!new_source.IsHealthy()) return nullptr;
    sources.push_back(new_source);
    return &sources.back();
}

void Manager::CleanUpSourceData(Source* src)
{
    if (!src) return;
    setListenWOUpdate(false);
    src->CleanUpData();
    setListenWOUpdate(true);
}

Source* Manager::GetSource(const FormID some_formid)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!some_formid) return nullptr;
    const auto some_form = GetFormByID(some_formid);
    if (!some_form) {
        logger::warn("Form not found.");
        return nullptr;
    }
    // maybe it already exists
    for (auto& src : sources) {
        if (!src.IsHealthy()) continue;
        if (src.IsStage(some_formid)) return &src;
    }

    if (IsDynamicFormID(some_formid)) {
        logger::trace("GetSource DynamicFormID: {}", some_formid);
        return nullptr;
    }

    // doesnt exist so we make new
    // registered stageler arasinda deil. yani fake de deil
    // custom stage mi deil mi onu anlamam lazim
    auto _qformtype = Settings::GetQFormType(some_formid);
    logger::trace("GetSource: QFormType: {}", _qformtype);
    if (Settings::custom_settings.empty()) {
        logger::trace("GetSource: custom settings is empty.");
    }
    if (!_qformtype.empty() && Settings::custom_settings.contains(_qformtype)) {
        auto& custom_settings = Settings::custom_settings[_qformtype];
        for (auto& [names, sttng] : custom_settings) {
            if (!sttng.IsHealthy()) continue;
            logger::trace("GetSource: custom settings owner names {}",
                            String::join(names, ", "));
            for (auto& name : names) {
                const FormID temp_cstm_formid = GetFormEditorIDFromString(name);
                if (temp_cstm_formid <= 0) continue;
                if (const auto temp_cstm_form = GetFormByID(temp_cstm_formid, name)) {
                    if (temp_cstm_form->GetFormID() == some_formid) return _MakeSource(some_formid, &sttng);
                }
            }
            if (String::includesWord(some_form->GetName(), names)) {
                return _MakeSource(some_formid, &sttng);
            }
        }
    }

    logger::trace("Source not found for {}", some_form->GetName());
    return nullptr;
}

Source* Manager::ForceGetSource(const FormID some_formid)
{
    auto src = GetSource(some_formid);  // also saves it to sources if it was created new
    // can be null if it is not a custom stage and not registered before
    if (!src) {
        logger::trace("No existing source and no custom settings found for the formid {}", some_formid);
        const auto qform_type = Settings::GetQFormType(some_formid);
        if (!Settings::IsItem(some_formid, qform_type, true)) {
            logger::trace("Not an item.");
            return nullptr;
        } else if (!Settings::defaultsettings.contains(qform_type)) {
            logger::trace("No default settings found for the qform_type {}", qform_type);
            return nullptr;
        } else if (!Settings::defaultsettings[qform_type].IsHealthy()) {
            logger::trace("Default settings not loaded for the qform_type {}", qform_type);
            return nullptr;
        } else {
            logger::trace("Creating new source for the formid {}", some_formid);
            src = _MakeSource(some_formid,
                                nullptr);  // stage item olarak dusunulduyse, custom a baslangic itemi olarak koymali
        }
    }
    return src;
}

StageInstance* Manager::GetWOStageInstance(RefID wo_refid)
{
    if (!wo_refid) {
        RaiseMngrErr("Ref is null.");
        return nullptr;
    }
    if (sources.empty()) return nullptr;
    for (auto& src : sources) {
        if (!src.data.contains(wo_refid)) continue;
        auto& instances = src.data[wo_refid];
        if (instances.size() == 1)
            return &instances[0];
        else if (instances.empty()) {
            logger::error("Stage instance found but empty.");
        } else if (instances.size() > 1) {
            logger::error("Multiple stage instances found.");
        }
    }
    logger::error("Stage instance not found.");
    return nullptr;
}

StageInstance* Manager::GetWOStageInstance(RE::TESObjectREFR* wo_ref)
{
    if (!wo_ref) {
        RaiseMngrErr("Ref is null.");
        return nullptr;
    }
    return GetWOStageInstance(wo_ref->GetFormID());
}

Source* Manager::GetWOSource(RefID wo_refid)
{
    if (!wo_refid) return nullptr;
    if (sources.empty()) {
        logger::error("GetWOSource: Sources is empty.");
        return nullptr;
    }
    for (auto& src : sources) {
        if (src.data.contains(wo_refid) && !src.data[wo_refid].empty()) {
            if (src.data[wo_refid].size() == 1)
                return &src;
            else if (src.data[wo_refid].size() > 1) {
                RaiseMngrErr("Multiple stage instances found.");
                return nullptr;
            }
        }
    }
    return nullptr;
}

Source* Manager::GetWOSource(RE::TESObjectREFR* wo_ref)
{
    if (!wo_ref) {
        RaiseMngrErr("Ref is null.");
        return nullptr;
    }
    return GetWOSource(wo_ref->GetFormID());
}

inline void Manager::_ApplyStageInWorld_Fake(RE::TESObjectREFR* wo_ref, const char* xname)
{
    if (!xname) {
        logger::error("ExtraTextDisplayData is null.");
        return;
    }
    logger::trace("Setting text display data for fake wo.");
    wo_ref->extraList.RemoveByType(RE::ExtraDataType::kTextDisplayData);
    auto xText = RE::BSExtraData::Create<RE::ExtraTextDisplayData>();
    xText->SetName(xname);
    logger::trace("{}", xText->displayName);
    wo_ref->extraList.Add(xText);
}

inline void Manager::_ApplyStageInWorld_Custom(RE::TESObjectREFR* wo_ref, RE::TESBoundObject* stage_bound)
{
    wo_ref->extraList.RemoveByType(RE::ExtraDataType::kTextDisplayData);
    logger::trace("Setting ObjectReference to custom stage form.");
    WorldObject::SwapObjects(wo_ref, stage_bound);
}

void Manager::ApplyStageInWorld(RE::TESObjectREFR* wo_ref, const Stage& stage, RE::TESBoundObject* source_bound)
{
    if (!source_bound) _ApplyStageInWorld_Custom(wo_ref, stage.GetBound());
    else {
        WorldObject::SwapObjects(wo_ref, source_bound);
        _ApplyStageInWorld_Fake(wo_ref, stage.GetExtraText());
    }
}

inline void Manager::_ApplyEvolutionInInventoryX(RE::TESObjectREFR* inventory_owner, Count update_count, FormID old_item, FormID new_item)
{
    logger::trace("Updating stage in inventory of {} Count {} , Old item {} , New item {}",
                      inventory_owner->GetName(), update_count, old_item, new_item);

    const auto inventory = inventory_owner->GetInventory();
    const auto entry = inventory.find(RE::TESForm::LookupByID<RE::TESBoundObject>(old_item));
    if (entry == inventory.end()) {
        logger::error("Item not found in inventory.");
        return;
    } else if (entry->second.second->IsQuestObject()) {
        logger::warn("Item is a quest object.");
        return;
    }
    bool has_xList = Inventory::EntryHasXData(entry->second.second.get());

    const auto __count = std::min(update_count, entry->second.first);
    auto ref_handle = WorldObject::DropObjectIntoTheWorld(
        RE::TESForm::LookupByID<RE::TESBoundObject>(new_item), __count);
    if (has_xList) {
        if (!xData::UpdateExtras(entry->second.second->extraLists->front(), &ref_handle->extraList)) {
            logger::info("ExtraDataList not updated.");
        }
    } else
        logger::info("original ExtraDataList is null.");

    setListenContainerChange(false);
    if (!WorldObject::PlayerPickUpObject(ref_handle, __count)) {
        logger::error("Item not picked up.");
        return;
    }
    setListenContainerChange(true);

    RemoveItemReverse(inventory_owner, nullptr, old_item, __count, RE::ITEM_REMOVE_REASON::kRemove);
    logger::trace("Stage updated in inventory.");
}

inline void Manager::_ApplyEvolutionInInventory(RE::TESObjectREFR* inventory_owner, Count update_count, FormID old_item, FormID new_item)
{
    if (!inventory_owner) return RaiseMngrErr("Inventory owner is null.");

    logger::trace("Updating stage in inventory of {} Count {} , Old item {} , New item {}",
                    inventory_owner->GetName(), update_count, old_item, new_item);

    if (update_count <= 0) {
        logger::error("Update count is 0 or less {}.", update_count);
        return;
    }

    const auto inventory = inventory_owner->GetInventory();
    auto* old_bound = RE::TESForm::LookupByID<RE::TESBoundObject>(old_item);
    if (!old_bound) {
        logger::error("Old item is null.");
        return;
    }
    const auto entry = inventory.find(old_bound);
    if (entry == inventory.end()) {
        logger::error("Item not found in inventory.");
        return;
    }
    const auto inv_count = entry->second.first;
    if (inv_count <= 0) {
        logger::warn("Item count in inventory is 0 or less {}.", inv_count);
        return;
    }
    if (!entry->second.second) {
        logger::error("Item data is null.");
        return;
    }
    if (entry->second.second->IsQuestObject()) {
        logger::warn("Item is a quest object.");
        return;
    }
    RemoveItemReverse(inventory_owner, nullptr, old_item, std::min(update_count, inv_count),
                        RE::ITEM_REMOVE_REASON::kRemove);
    AddItem(inventory_owner, nullptr, new_item, update_count);
    logger::trace("Stage updated in inventory.");
}


void Manager::ApplyEvolutionInInventory(std::string _qformtype_, RE::TESObjectREFR* inventory_owner, Count update_count, FormID old_item, FormID new_item)
{
    if (!inventory_owner) return RaiseMngrErr("Inventory owner is null.");
    if (!old_item || !new_item) return RaiseMngrErr("Item is null.");
    if (!update_count) {
        logger::warn("Update count is 0.");
        return;
    }
    if (old_item == new_item) {
        logger::trace("ApplyEvolutionInInventory: New item is the same as the old item.");
        return;
    }

    bool is_faved = false;
    bool is_equipped = false;
    if (inventory_owner->IsPlayerRef()) {
        is_faved = IsPlayerFavorited(RE::TESForm::LookupByID<RE::TESBoundObject>(old_item));
        is_equipped = IsEquipped(RE::TESForm::LookupByID<RE::TESBoundObject>(old_item));
    }
    if (Vector::HasElement<std::string>(Settings::xQFORMS, _qformtype_)) {
        _ApplyEvolutionInInventoryX(inventory_owner, update_count, old_item, new_item);
    } else if (is_faved || is_equipped) {
        _ApplyEvolutionInInventoryX(inventory_owner, update_count, old_item, new_item);
    } else
        _ApplyEvolutionInInventory(inventory_owner, update_count, old_item, new_item);

    if (is_faved) FavoriteItem(RE::TESForm::LookupByID<RE::TESBoundObject>(new_item),
                                                            inventory_owner);
    if (is_equipped) {
        setListenEquip(false);
        EquipItem(RE::TESForm::LookupByID<RE::TESBoundObject>(new_item));
        setListenEquip(true);
    }
}

inline const RE::ObjectRefHandle Manager::RemoveItemReverse(RE::TESObjectREFR* moveFrom, RE::TESObjectREFR* moveTo, FormID item_id, Count count, RE::ITEM_REMOVE_REASON reason)
{
    logger::trace("Removing item reverse");

    auto ref_handle = RE::ObjectRefHandle();

    if (count <= 0) {
        logger::warn("Count is 0 or less.");
        return ref_handle;
    }

    if (!moveFrom && !moveTo) {
        RaiseMngrErr("moveFrom and moveTo are both null!");
        return ref_handle;
    }
    if (moveFrom && moveTo && moveFrom->GetFormID() == moveTo->GetFormID()) {
        logger::info("moveFrom and moveTo are the same!");
        return ref_handle;
    }

    logger::trace("RemoveItemReverse {} from {}", item_id, moveFrom->GetFormID());
    setListenContainerChange(false);

    auto inventory = moveFrom->GetInventory();
    for (auto item = inventory.rbegin(); item != inventory.rend(); ++item) {
        auto item_obj = item->first;
        if (!item_obj) RaiseMngrErr("Item object is null");
        if (item_obj->GetFormID() == item_id) {
            auto inv_data = item->second.second.get();
            if (inv_data->IsQuestObject()) {
                logger::warn("Item is a quest object.");
                return ref_handle;
            }
            if (!inv_data) RaiseMngrErr("Item data is null");
            auto asd = inv_data->extraLists;
            if (!asd || asd->empty()) {
                logger::trace("Removing item reverse without extra data.");
                ref_handle = moveFrom->RemoveItem(item_obj, count, reason, nullptr, moveTo);
            } else {
                logger::trace("Removing item reverse with extra data.");
                ref_handle = moveFrom->RemoveItem(item_obj, count, reason, asd->front(), moveTo);
            }
            // Utilities::FunctionsSkyrim::Menu::SendInventoryUpdateMessage(moveFrom, item_obj);
            break;
        }
    }

    setListenContainerChange(true);
    return ref_handle;
}

void Manager::AddItem(RE::TESObjectREFR* addTo, RE::TESObjectREFR* addFrom, FormID item_id, Count count)
{
    logger::trace("AddItem");
    // xList = nullptr;
    if (!addTo) return RaiseMngrErr("add to is null!");
    if (count <= 0) {
        logger::error("Count is 0 or less.");
        return;
    }
    if (addFrom) {
        if (addTo->GetFormID() == addFrom->GetFormID()) {
            logger::warn("Add to and add from are the same.");
            return;
        } else if (!addFrom->HasContainer()) {
            logger::warn("Add from does not have a container.");
            return;
        }
    }

    logger::trace("Adding item.");

    auto* bound = RE::TESForm::LookupByID<RE::TESBoundObject>(item_id);
    if (!bound) {
        logger::critical("Bound is null.");
        return;
    }
    logger::trace("setting listen container change to false.");
    setListenContainerChange(false);
    logger::trace("Adding item to container.");
    addTo->AddObjectToContainer(bound, nullptr, count, addFrom);
    logger::trace("Item added to container.");
    setListenContainerChange(true);
    logger::trace("listen container change set to true.");
    SKSE::GetTaskInterface()->AddTask([addTo, bound]() {
        logger::trace("Refreshing inventory for newly added item.");
        RE::SendUIMessage::SendInventoryUpdateMessage(addTo, bound);
    });
    // Utilities::FunctionsSkyrim::Menu::SendInventoryUpdateMessage(addTo, bound);
}

bool Manager::_UpdateStagesInSource(std::vector<RE::TESObjectREFR*> refs, Source& src, const float curr_time)
{
    if (!src.IsHealthy()) {
        RaiseMngrErr("_UpdateStages: Source is not healthy.");
        return false;
    }
    if (refs.empty()) {
        RaiseMngrErr("_UpdateStages: Refs is empty.");
        return false;
    }

    logger::trace("_UpdateStagesInSource");
    std::map<RefID, RE::TESObjectREFR*> ids_refs;
    std::vector<RefID> refids;
    for (auto& ref : refs) {
        if (!ref) {
            RaiseMngrErr("_UpdateStages: ref is null.");
            return false;
        }
        ids_refs[ref->GetFormID()] = ref;
        refids.push_back(ref->GetFormID());
    }
    auto updated_stages = src.UpdateAllStages(refids, curr_time);
    if (updated_stages.empty()) {
        logger::trace("No update2");
        return false;
    }

    // std::set<RefID> decayed_was_registered;
    for (const auto& [loc, updates] : updated_stages) {
        for (const auto& update : updates) {
            if (!update.oldstage || !update.newstage || !update.count) {
                logger::error("_UpdateStages: Update is null.");
                continue;
            }
            auto ref = ids_refs[loc];
            if (ref->HasContainer() || ref->IsPlayerRef()) {
                ApplyEvolutionInInventory(src.qFormType, ref, update.count, update.oldstage->formid,
                                            update.newstage->formid);
                if (src.IsDecayedItem(update.newstage->formid)) {
                    to_register_go.push_back({update.newstage->formid, update.count, loc, update.update_time});
                }
            }
            // WO
            else if (worldobjectsevolve) {
                logger::trace("_UpdateStages: ref out in the world.");
                auto bound = src.IsFakeStage(update.newstage->no) ? src.GetBoundObject() : nullptr;
                ApplyStageInWorld(ref, *update.newstage, bound);
                if (src.IsDecayedItem(update.newstage->formid)) {
                    logger::trace("_UpdateStages: Decayed item. Source formid {} editorid {}", src.formid,
                                    src.editorid);
                    to_register_go.push_back({update.newstage->formid, update.count, loc, update.update_time});
                    if (!src.IsHealthy()) {
                        RaiseMngrErr("_UpdateStages: Source is not healthy7.");
                        return false;
                    }
                }
            } else {
                logger::critical("_UpdateStages: Unknown ref type.");
                return false;
            }
        }
    }
    if (!src.IsHealthy()) {
        RaiseMngrErr("_UpdateStages: Source is not healthy8.");
        return false;
    }

    // src->CleanUpData();
    return true;
}

bool Manager::_UpdateStagesOfRef(RE::TESObjectREFR* ref, const float _time, bool is_inventory)
{
    if (!ref) {
        RaiseMngrErr("_UpdateStagesOfRef: Ref is null.");
        return false;
    }

    logger::trace("_UpdateStagesOfRef");
    const RefID refid = ref->GetFormID();
    bool update_took_place = false;
    if (!is_inventory) {
        auto src_ = GetWOSource(ref);
        if (!src_) {
            logger::error("Source not found for the ref.");
            return false;
        }
        Source& src = *src_;
        if (!src.IsHealthy()) {
            logger::error("Source is not healthy.");
            return false;
        }
        if (src.data.empty()) {
            logger::error("Source data is empty.");
            return false;
        }
        if (!src.data.contains(refid)) {
            logger::critical("Source data does not contain refid.");
            return false;
        }
        if (src.data.at(refid).empty()) {
            logger::warn("Source data is empty for refid.");
            return false;
        }
        if (!src.IsHealthy()) {
            RaiseMngrErr("_UpdateStagesOfRef: Source is not healthy1.");
            return false;
        }
        update_took_place = _UpdateStagesInSource({ref}, src, _time);

        if (!src.IsHealthy()) {
            RaiseMngrErr("_UpdateStagesOfRef: Source is not healthy2.");
            return false;
        }

        if (auto wo_inst = GetWOStageInstance(refid); wo_inst) {
            const auto curr_time = RE::Calendar::GetSingleton()->GetHoursPassed();
            const auto next_update = src.GetNextUpdateTime(wo_inst);
            if (next_update > curr_time) QueueWOUpdate(refid, next_update);
        }

        return update_took_place;
    }

    for (auto& src : sources) {
        if (!src.IsHealthy()) continue;
        if (src.data.empty()) continue;
        if (!src.data.contains(refid)) continue;
        if (src.data[refid].empty()) continue;
        const bool temp_update_took_place = _UpdateStagesInSource({ref}, src, _time);
        if (!update_took_place) update_took_place = temp_update_took_place;
    }
    for (auto& src : sources) {
        if (!src.IsHealthy()) continue;
        if (src.data.empty()) continue;
        if (!src.data.contains(refid)) continue;
        if (src.data[refid].empty()) continue;
        if (is_inventory) src.UpdateTimeModulationInInventory(ref, _time);
    }
    return update_took_place;
}

bool Manager::_UpdateTimeModulators(RE::TESObjectREFR* inventory_owner, const float curr_time)
{
    logger::trace("_UpdateTimeModulators");

    if (!inventory_owner) {
        RaiseMngrErr("_UpdateTimeModulators: Inventory owner is null.");
        return false;
    }

    const RefID inventory_owner_refid = inventory_owner->GetFormID();

    // std::vector<StageInstance*> all_instances_in_inventory;
    std::vector<float> queued_updates;  // pair: source owner index , hitting time
    // since these instances are in the same inventory, they are affected by the same time modulator

    std::set<FormID> all_time_modulators;
    for (auto& src : sources) {
        if (!src.data.contains(inventory_owner_refid)) continue;
        auto& temp_delayers = src.defaultsettings->delayers;
        for (auto& temp_dlyr : temp_delayers) {
            logger::trace("Time modulator: {}", temp_dlyr.first);
            all_time_modulators.insert(temp_dlyr.first);
        }
    }
    for (auto& src : sources) {
        if (!src.IsHealthy()) {
            RaiseMngrErr("_UpdateTimeModulators: Source is not healthy.");
        }
        if (!src.data.contains(inventory_owner_refid)) continue;
        for (auto& st_inst : src.data.at(inventory_owner_refid)) {
            if (st_inst.xtra.is_decayed || !src.IsStageNo(st_inst.no)) continue;
            if (all_time_modulators.contains(st_inst.xtra.form_id)) {
                const auto delay_slope = st_inst.GetDelaySlope();
                if (delay_slope == 0) continue;
                const auto schranke = delay_slope > 0 ? src.GetStage(st_inst.no).duration : 0.f;
                logger::trace("getting hitting time for {} with source formid {}", st_inst.xtra.form_id,
                                src.formid);
                const auto hitting_time = st_inst.GetHittingTime(schranke);
                if (hitting_time <= 0) {
                    logger::warn("Hitting time is 0 or less!");
                    continue;
                } else if (hitting_time <= curr_time)
                    queued_updates.push_back(hitting_time);
            }
        }
    }

    all_time_modulators.clear();

    if (queued_updates.empty()) {
        logger::trace("No queued updates.");
        return false;
    } else
        logger::trace("Queued updates: {}", queued_updates.size());

    // order them by hitting time
    logger::trace("Sorting queued updates.");
    std::sort(queued_updates.begin(), queued_updates.end(), [](float a, float b) { return a < b; });

    logger::trace("Applying queued updates.");
    for (auto& q_u : queued_updates) {
        const auto _t_ = q_u + std::numeric_limits<float>::epsilon();
        if (_UpdateStagesOfRef(inventory_owner, _t_, true)) {
            logger::trace("Time modulator updated.");
            _UpdateTimeModulators(inventory_owner, curr_time);
            return true;
        }
    }

    logger::trace("_UpdateTimeModulators: Done.");

    return false;
}

void Manager::RaiseMngrErr(const std::string err_msg_)
{
    logger::critical("{}", err_msg_);
    MsgBoxesNotifs::InGame::CustomMsg(err_msg_);
    MsgBoxesNotifs::InGame::GeneralErr();
    Uninstall();
}

void Manager::InitFailed()
{
    logger::critical("Failed to initialize Manager.");
    MsgBoxesNotifs::InGame::InitErr();
    Uninstall();
    return;
}

void Manager::Init()
{
    bool init_failed = false;

    po3_use_or_take = IsPo3_UoTInstalled();

    if (Settings::INI_settings.contains("Other Settings")) {
        if (Settings::INI_settings["Other Settings"].contains("WorldObjectsEvolve")) {
            worldobjectsevolve = Settings::INI_settings["Other Settings"]["WorldObjectsEvolve"];
        } else
            logger::warn("WorldObjectsEvolve not found.");
        if (Settings::INI_settings["Other Settings"].contains("bReset")) {
            should_reset = Settings::INI_settings["Other Settings"]["bReset"];
        } else
            logger::warn("bReset not found.");
    } else
        logger::critical("Other Settings not found.");

    _instance_limit = Settings::nMaxInstances;

    if (init_failed) InitFailed();
    logger::info("Manager initialized with instance limit {}", _instance_limit);

    // add safety check for the sources size say 5 million
}

void Manager::setListenEquip(const bool value)
{
    std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    listen_equip = value;
}

void Manager::setListenWOUpdate(const bool value)
{
    std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    listen_woupdate = value;
}

void Manager::setListenContainerChange(const bool value)
{
    std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    listen_container_change = value;
}

const bool Manager::getListenWOUpdate()
{
    std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    return listen_woupdate;
}

void Manager::setUpdateIsBusy(const bool)
{
    return;
    // std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    // update_is_busy = value;
}

const bool Manager::getUpdateIsBusy()
{
    std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
    return update_is_busy;
}

const bool Manager::RefIsRegistered(const RefID refid)
{
    if (!refid) {
        logger::warn("Refid is null.");
        return false;
    }
    if (sources.empty()) {
        logger::warn("Sources is empty.");
        return false;
    }
    for (auto& src : sources) {
        if (src.data.contains(refid) && !src.data[refid].empty()) return true;
    }
    return false;
}

const bool Manager::RegisterAndGo(const FormID some_formid, const Count count, const RefID location_refid, Duration register_time)
{
    if (!some_formid) {
        logger::warn("Formid is null.");
        return false;
    }
    if (!count) {
        logger::warn("Count is 0.");
        return false;
    }
    if (!location_refid) {
        RaiseMngrErr("Location refid is null.");
        return false;
    }
    if (do_not_register.contains(some_formid)) {
        logger::trace("Formid is in do not register list.");
        return false;
    }

    if (GetNInstances() > _instance_limit) {
        logger::warn("Instance limit reached.");
        MsgBoxesNotifs::InGame::CustomMsg(
            std::format("The mod is tracking over {} instances. Maybe it is not bad to check your memory usage and "
                        "skse co-save sizes.",
                        _instance_limit));
    }
    if (IsDynamicFormID(some_formid)) {
        logger::trace("Skipping fake form at source creation.");
        return false;
    }

    logger::trace("Registering new instance.Formid {} , Count {} , Location refid {}, register_time {}",
                    some_formid, count, location_refid, register_time);
    // make new registry
    auto src = ForceGetSource(some_formid);  // also saves it to sources if it was created new
    if (!src) {
        // RaiseMngrErr("Register: Source is null.");
        logger::trace("Register: Source is null.");
        do_not_register.insert(some_formid);
        return false;
    } else if (!src->IsStage(some_formid)) {
        logger::critical("Register: some_formid is not a stage.");
        // do_not_register.insert(some_formid);
        return false;
    }

    const auto stage_no = src->formid == some_formid ? 0 : src->GetStageNo(some_formid);
    auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(location_refid);
    if (!ref) {
        RaiseMngrErr("Register: Ref is null.");
        return false;
    }

    if (register_time == 0) register_time = RE::Calendar::GetSingleton()->GetHoursPassed();
    if (ref->HasContainer() || location_refid == player_refid) {
        logger::trace("Registering in inventory.");
        if (!src->InitInsertInstanceInventory(stage_no, count, ref, register_time)) {
            RaiseMngrErr("Register: InsertNewInstance failed 1.");
            return false;
        }
        const auto stage_formid = src->GetStage(stage_no).formid;
        // to change from the source form to the stage form
        ApplyEvolutionInInventory(src->qFormType, ref, count, some_formid, stage_formid);
    } else {
        logger::trace("Registering in world. name {} formid {}", ref->GetObjectReference()->GetName(),
                        ref->GetObjectReference()->GetFormID());
        if (!src->InitInsertInstanceWO(stage_no, count, location_refid, register_time)) {
            RaiseMngrErr("Register: InsertNewInstance failed 1.");
            return false;
        }
        auto bound = src->IsFakeStage(stage_no) ? src->GetBoundObject() : nullptr;
        /*for (const auto& [no,stg] : src->stages) {
            logger::trace("Stage no {} formid {}, is fake {}",stg.no,stg.formid,src->IsFakeStage(stg.no));
        }*/
        ApplyStageInWorld(ref, src->GetStage(stage_no), bound);
    }

    UpdateStages(location_refid);

    return true;
}

const bool Manager::RegisterAndGo(RE::TESObjectREFR* wo_ref)
{
    if (!worldobjectsevolve) return false;
    if (!wo_ref) {
        logger::warn("Ref is null.");
        return false;
    }
    const auto wo_refid = wo_ref->GetFormID();
    if (!wo_refid) {
        logger::warn("Refid is null.");
        return false;
    }
    const auto wo_bound = wo_ref->GetBaseObject();
    if (!wo_bound) {
        logger::warn("Bound is null.");
        return false;
    }
    if (RefIsRegistered(wo_refid)) {
        logger::warn("Ref is already registered.");
        return false;
    }
    return RegisterAndGo(wo_bound->GetFormID(), wo_ref->extraList.GetCount(), wo_refid);
}

bool Manager::UpdateStages(RE::TESObjectREFR* ref, const float _time)
{
    if (getUpdateIsBusy()) {
        logger::critical("UpdateStages: Update is busy.");
        return false;
    }
    setUpdateIsBusy(true);
    // std::lock_guard<std::mutex> lock(mutex);
    //  assumes that the ref is registered
    logger::trace("Manager: Updating stages.");
    if (!ref) {
        logger::critical("UpdateStages: ref is null.");
        setUpdateIsBusy(false);
        return false;
    }
    if (sources.empty()) {
        logger::trace("UpdateStages: Sources is empty.");
        setUpdateIsBusy(false);
        return false;
    }

    const auto curr_time = _time ? _time : RE::Calendar::GetSingleton()->GetHoursPassed();

    logger::trace("UpdateStages: curr_time: {}", curr_time);

    // time modulator updates
    const bool is_inventory = ref->HasContainer() || ref->IsPlayerRef();
    bool update_took_place = false;
    if (is_inventory) {
        if (locs_to_be_handled.contains(ref->GetFormID())) _HandleLoc(ref);
        // if there are time modulators which can also evolve, they need to be updated first
        const bool temp_update_took_place = _UpdateTimeModulators(ref, curr_time);
        if (!update_took_place) update_took_place = temp_update_took_place;
    } else if (!worldobjectsevolve) {
        setUpdateIsBusy(false);
        return false;
    }

    // need to handle queued_time_modulator_updates
    // order them by GetRemainingTime method of QueuedTModUpdate
    const bool temp_update_took_place = _UpdateStagesOfRef(ref, curr_time, is_inventory);
    logger::trace("UpdateStages: temp_update_took_place: {}", temp_update_took_place);
    if (!update_took_place) update_took_place = temp_update_took_place;
    if (!update_took_place) logger::trace("No update1");

    logger::trace("while (!to_register_go.empty())");
    while (!to_register_go.empty()) {
        const auto& [formid, count, refid, r_time] = to_register_go.front();
        to_register_go.erase(to_register_go.begin());
        if (!RegisterAndGo(formid, count, refid, r_time)) {
            logger::trace("UpdateStages: RegisterAndGo failed for form {} and loc {}.", formid, refid);
        }
    }

    for (auto& src : sources) {
        if (src.data.empty()) continue;
        CleanUpSourceData(&src);
    }

#ifndef NDEBUG
    Print();
#endif  // !NDEBUG

    setUpdateIsBusy(false);

    return update_took_place;
}

bool Manager::UpdateStages(RefID loc_refid)
{
    logger::trace("Manager: Updating stages for loc_refid {}.", loc_refid);
    if (!loc_refid) {
#ifndef NDEBUG
        logger::warn("UpdateStages: loc_refid is null.");
#endif
        return false;
    }

    auto loc_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(loc_refid);
    if (!loc_ref) {
        logger::critical("UpdateStages: loc_ref is null.");
        return false;
    }
    return UpdateStages(loc_ref);
}

void Manager::_HandleLoc(RE::TESObjectREFR* loc_ref)
{
    if (!loc_ref) {
        logger::error("Loc ref is null.");
        return;
    }
    const auto loc_refid = loc_ref->GetFormID();

    if (!locs_to_be_handled.contains(loc_refid)) {
        logger::trace("Loc ref not in locs_to_be_handled.");
        return;
    }

    if (!loc_ref->HasContainer() && !loc_ref->IsPlayerRef()) {
        logger::trace("Does not have container");
        // remove the loc refid key from locs_to_be_handled map
        auto it = locs_to_be_handled.find(loc_refid);
        if (it != locs_to_be_handled.end()) {
            locs_to_be_handled.erase(it);
        }
        return;
    }

    const auto loc_inventory_temp = loc_ref->GetInventory();
    for (const auto& [bound, entry] : loc_inventory_temp) {
        if (bound && IsDynamicFormID(bound->GetFormID()) &&
            std::strlen(bound->GetName()) == 0) {
            RemoveItemReverse(loc_ref, nullptr, bound->GetFormID(), std::max(1, entry.first),
                                RE::ITEM_REMOVE_REASON::kRemove);
        }
    }

    // handle discrepancies in inventory vs registries
    std::map<FormID, std::vector<StageInstance*>> formid_instances_map = {};
    std::map<FormID, Count> total_registry_counts = {};
    for (auto& src : sources) {
        if (src.data.empty()) continue;
        if (!src.data.contains(loc_refid)) continue;
        for (auto& st_inst : src.data[loc_refid]) {  // bu liste onceski savele ayni deil cunku source.datayi
                                                        // _registeratreceivedata deistirdi
            if (st_inst.xtra.is_decayed) continue;
            if (st_inst.count <= 0) continue;
            const auto temp_formid = st_inst.xtra.form_id;
            formid_instances_map[temp_formid].push_back(&st_inst);
            if (!total_registry_counts.contains(temp_formid))
                total_registry_counts[temp_formid] = st_inst.count;
            else
                total_registry_counts[temp_formid] += st_inst.count;
        }
    }

    // for every formid, handle the discrepancies
    const auto loc_inventory = loc_ref->GetInventory();  // formids are unique so i can pull it out of for-loop
    for (auto& [formid, instances] : formid_instances_map) {
        auto total_registry_count = total_registry_counts[formid];
        const auto it = loc_inventory.find(GetFormByID<RE::TESBoundObject>(formid));
        const auto inventory_count = it != loc_inventory.end() ? it->second.first : 0;
        auto diff = total_registry_count - inventory_count;
        if (diff < 0) {
            logger::warn("HandleLoc: Something could have gone wrong with registration.");
            continue;
        }
        if (diff == 0) {
            logger::info("HandleLoc: Nothing to remove.");
            continue;
        }
        for (auto& instance : instances) {
            if (const auto bound = GetFormByID<RE::TESBoundObject>(formid);
                bound && instance->xtra.is_fake) {
                AddItem(loc_ref, nullptr, formid, diff);
                break;
            } else if (diff <= instance->count) {
                instance->count -= diff;
                break;
            } else {
                diff -= instance->count;
                instance->count = 0;
            }
        }
    }

    auto it = locs_to_be_handled.find(loc_refid);
    if (it != locs_to_be_handled.end()) {
        locs_to_be_handled.erase(it);
    }

    logger::trace("HandleLoc: synced with loc {}.", loc_refid);
}

void Manager::Print()
{
    logger::info("Printing sources...Current time: {}", RE::Calendar::GetSingleton()->GetHoursPassed());
    for (auto& src : sources) {
        if (src.data.empty()) continue;
        src.PrintData();
    }
}
