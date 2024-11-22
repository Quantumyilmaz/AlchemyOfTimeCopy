#include "Utils.h"

#include <numbers>

bool Types::FormEditorID::operator<(const FormEditorID& other) const
{
    // Compare form_id first
    if (form_id < other.form_id) {
        return true;
    }
    // If form_id is equal, compare editor_id
    if (form_id == other.form_id && editor_id < other.editor_id) {
        return true;
    }
    // If both form_id and editor_id are equal or if form_id is greater, return false
    return false;
}

bool Types::FormEditorIDX::operator==(const FormEditorIDX& other) const
{
    return form_id == other.form_id;
}

void SetupLog()
{
    const auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    const auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
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

std::filesystem::path GetLogPath()
{
    const auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    return logFilePath;
}

std::vector<std::string> ReadLogFile()
{
    std::vector<std::string> logLines;

    // Open the log file
    std::ifstream file(GetLogPath().c_str());
    if (!file.is_open()) {
        // Handle error
        return logLines;
    }

    // Read and store each line from the file
    std::string line;
    while (std::getline(file, line)) {
        logLines.push_back(line);
    }

    file.close();

    return logLines;
}

std::string DecodeTypeCode(std::uint32_t typeCode)
{
    char buf[4];
    buf[3] = char(typeCode);
    buf[2] = char(typeCode >> 8);
    buf[1] = char(typeCode >> 16);
    buf[0] = char(typeCode >> 24);
    return std::string(buf, buf + 4);
}

bool FileIsEmpty(const std::string& filename)
{
	std::ifstream file(filename);
    if (!file.is_open()) {
        return false;  // File could not be opened, treat as not empty or handle error
    }

    char ch;
    while (file.get(ch)) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            return false;  // Found a non-whitespace character
        }
    }

    return true;  // Only whitespace characters or file is empty
}


bool isValidHexWithLength7or8(const char* input)
{
    std::string inputStr(input);

    if (inputStr.substr(0, 2) == "0x") {
        // Remove "0x" from the beginning of the string
        inputStr = inputStr.substr(2);
    }

    const std::regex hexRegex("^[0-9A-Fa-f]{7,8}$");  // Allow 7 to 8 characters
    const bool isValid = std::regex_match(inputStr, hexRegex);
    return isValid;
}

std::string GetEditorID(const FormID a_formid) {
    if (const auto form = RE::TESForm::LookupByID(a_formid)) {
        return clib_util::editorID::get_editorID(form);
    } else {
        return "";
    }
}

FormID GetFormEditorIDFromString(const std::string& formEditorId)
{
    if (isValidHexWithLength7or8(formEditorId.c_str())) {
        int form_id_;
        std::stringstream ss;
        ss << std::hex << formEditorId;
        ss >> form_id_;
        if (const auto temp_form = GetFormByID(form_id_, "")) return temp_form->GetFormID();
        logger::error("Formid is null for editorid {}", formEditorId);
        return 0;
    }
    if (formEditorId.empty()) return 0;
    if (!IsPo3Installed()) {
        logger::error("Po3 is not installed.");
        MsgBoxesNotifs::Windows::Po3ErrMsg();
        return 0;
    }
    if (const auto temp_form = GetFormByID(0, formEditorId)) return temp_form->GetFormID();
    return 0;
}

inline bool FormIsOfType(const RE::TESForm* form, RE::FormType type)
{
    if (!form) return false;
	return form->GetFormType() == type;
}

bool IsFoodItem(const RE::TESForm* form)
{
    if (FormIsOfType(form,RE::AlchemyItem::FORMTYPE)){
        const RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
        if (!form_as_) return false;
        if (!form_as_->IsFood()) return false;
    }
    else if (FormIsOfType(form,RE::IngredientItem::FORMTYPE)){
        const RE::IngredientItem* form_as_ = form->As<RE::IngredientItem>();
        if (!form_as_) return false;
        if (!form_as_->IsFood()) return false;

    }
    //else if(FormIsOfType(form,RE::MagicItem::FORMTYPE)){
    //    RE::MagicItem* form_as_ = form->As<RE::MagicItem>();
    //    if (!form_as_) return false;
    //    if (!form_as_->IsFood()) return false;
    //}
    else return false;
    return true;
}

bool IsPoisonItem(const RE::TESForm* form)
{
    if (FormIsOfType(form, RE::AlchemyItem::FORMTYPE)) {
        const RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
        if (!form_as_) return false;
        if (!form_as_->IsPoison()) return false;
    }
    else return false;
    return true;
}

bool IsMedicineItem(const RE::TESForm* form)
{
    if (FormIsOfType(form, RE::AlchemyItem::FORMTYPE)) {
        const RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
        if (!form_as_) return false;
        if (!form_as_->IsMedicine()) return false;
    } 
    else return false;
    return true;
}

void OverrideMGEFFs(RE::BSTArray<RE::Effect*>& effect_array, const std::vector<FormID>& new_effects, const std::vector<uint32_t>& durations, const std::vector<float>& magnitudes)
{
    size_t some_index = 0;
    for (auto* effect : effect_array) {
        if (auto* other_eff = GetFormByID<RE::EffectSetting>(new_effects[some_index]); !other_eff){
            effect->effectItem.duration = 0;
            effect->effectItem.magnitude = 0;
        }
        else {
			effect->baseEffect = other_eff;
			effect->effectItem.duration = durations[some_index];
			effect->effectItem.magnitude = magnitudes[some_index];
		}
        some_index++;
	}     
}

void FavoriteItem(const RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner)
{
    if (!item) return;
    if (!inventory_owner) return;
    const auto inventory_changes = inventory_owner->GetInventoryChanges();
    const auto entries = inventory_changes->entryList;
    for (auto it = entries->begin(); it != entries->end(); ++it) {
        if (!(*it)) {
			logger::error("Item entry is null");
			continue;
		}
        const auto object = (*it)->object;
        if (!object) {
			logger::error("Object is null");
			continue;
		}
        const auto formid = object->GetFormID();
        if (!formid) logger::critical("Formid is null");
        if (formid == item->GetFormID()) {
            logger::trace("Favoriting item: {}", item->GetName());
            const auto xLists = (*it)->extraLists;
            bool no_extra_ = false;
            if (!xLists || xLists->empty()) {
				logger::trace("No extraLists");
                no_extra_ = true;
			}
            logger::trace("asdasd");
            if (no_extra_) {
                logger::trace("No extraLists");
                //inventory_changes->SetFavorite((*it), nullptr);
            } else if (xLists->front()) {
                logger::trace("ExtraLists found");
                inventory_changes->SetFavorite((*it), xLists->front());
            }
            return;
        }
    }
    logger::error("Item not found in inventory");
}

bool IsFavorited(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner) {
    if (!item) {
        logger::warn("Item is null");
        return false;
    }
    if (!inventory_owner) {
        logger::warn("Inventory owner is null");
        return false;
    }
    auto inventory = inventory_owner->GetInventory();
    if (const auto it = inventory.find(item); it != inventory.end()) {
        if (it->second.first <= 0) logger::warn("Item count is 0");
        return it->second.second->IsFavorited();
    }
    return false;
}

void EquipItem(const RE::TESBoundObject* item, const bool unequip)
{
    logger::trace("EquipItem");

    if (!item) {
        logger::error("Item is null");
        return;
    }
    const auto player_ref = RE::PlayerCharacter::GetSingleton();
    const auto inventory_changes = player_ref->GetInventoryChanges();
    const auto entries = inventory_changes->entryList;
    for (auto it = entries->begin(); it != entries->end(); ++it) {
        if (const auto formid = (*it)->object->GetFormID(); formid == item->GetFormID()) {
            if (!(*it) || !(*it)->extraLists) {
				logger::error("Item extraLists is null");
				return;
			}
            if (unequip) {
                if ((*it)->extraLists->empty()) {
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(
                        player_ref, (*it)->object, nullptr, 1,
                        (const RE::BGSEquipSlot*)nullptr, true, false, false);
                } else if ((*it)->extraLists->front()) {
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(
                        player_ref, (*it)->object, (*it)->extraLists->front(), 1,
                        (const RE::BGSEquipSlot*)nullptr, true, false, false);
                }
            } else {
                if ((*it)->extraLists->empty()) {
                    RE::ActorEquipManager::GetSingleton()->EquipObject(
                        player_ref, (*it)->object, nullptr, 1,
                        (const RE::BGSEquipSlot*)nullptr, true, false, false, false);
                } else if ((*it)->extraLists->front()) {
                    RE::ActorEquipManager::GetSingleton()->EquipObject(
                        player_ref, (*it)->object, (*it)->extraLists->front(), 1,
                        (const RE::BGSEquipSlot*)nullptr, true, false, false, false);
                }
            }
            return;
        }
    }
}

bool IsEquipped(RE::TESBoundObject* item) {
    logger::trace("IsEquipped");

    if (!item) {
        logger::trace("Item is null");
        return false;
    }

    const auto player_ref = RE::PlayerCharacter::GetSingleton();
    auto inventory = player_ref->GetInventory();
    const auto it = inventory.find(item);
    if (it != inventory.end()) {
        if (it->second.first <= 0) logger::warn("Item count is 0");
        return it->second.second->IsWorn();
    }
    return false;
}

int16_t WorldObject::GetObjectCount(RE::TESObjectREFR* ref) {
    if (!ref) {
        logger::error("Ref is null.");
        return 0;
    }
    if (ref->extraList.HasType(RE::ExtraDataType::kCount)) {
        const RE::ExtraCount* xCount = ref->extraList.GetByType<RE::ExtraCount>();
        return xCount->count;
    }
    return 0;
}

void WorldObject::SetObjectCount(RE::TESObjectREFR* ref, Count count)
{
    if (!ref) {
        logger::error("Ref is null.");
        return;
    }
    int max_try = 1;
    while (ref->extraList.HasType(RE::ExtraDataType::kCount) && max_try) {
        ref->extraList.RemoveByType(RE::ExtraDataType::kCount);
        max_try--;
    }
    // ref->extraList.SetCount(static_cast<uint16_t>(count));
    const auto xCount = new RE::ExtraCount(static_cast<int16_t>(count));
    ref->extraList.Add(xCount);
}

RE::TESObjectREFR* WorldObject::DropObjectIntoTheWorld(RE::TESBoundObject* obj, const Count count, const bool player_owned)
{
    const auto player_ch = RE::PlayerCharacter::GetSingleton();

    constexpr auto multiplier = 100.0f;
    constexpr float q_pi = std::numbers::pi_v<float>;
    auto orji_vec = RE::NiPoint3{multiplier, 0.f, player_ch->GetHeight()};
    Math::LinAlg::R3::rotateZ(orji_vec, q_pi / 4.f - player_ch->GetAngleZ());
    const auto drop_pos = player_ch->GetPosition() + orji_vec;
    const auto player_cell = player_ch->GetParentCell();
    const auto player_ws = player_ch->GetWorldspace();
    if (!player_cell && !player_ws) {
        logger::critical("Player cell AND player world is null.");
        return nullptr;
    }
    const auto newPropRef =
        RE::TESDataHandler::GetSingleton()
                            ->CreateReferenceAtLocation(obj, drop_pos, {0.0f, 0.0f, 0.0f}, player_cell,
                                                        player_ws, nullptr, nullptr, {}, false, false)
            .get()
            .get();
    if (!newPropRef) {
        logger::critical("New prop ref is null.");
        return nullptr;
    }
    if (player_owned) newPropRef->extraList.SetOwner(RE::TESForm::LookupByID(0x07));
	if (count > 1) newPropRef->extraList.SetCount(static_cast<uint16_t>(count));
    return newPropRef;
}

void WorldObject::SwapObjects(RE::TESObjectREFR* a_from, RE::TESBoundObject* a_to, const bool apply_havok)
{
    logger::trace("SwapObjects");
    if (!a_from) {
        logger::error("Ref is null.");
        return;
    }
    const auto ref_base = a_from->GetBaseObject();
    if (!ref_base) {
        logger::error("Ref base is null.");
		return;
    }
    if (!a_to) {
		logger::error("Base is null.");
		return;
	}
    if (ref_base->GetFormID() == a_to->GetFormID()) {
		logger::trace("Ref and base are the same.");
		return;
	}
    a_from->SetObjectReference(a_to);
    if (!apply_havok) return;
    SKSE::GetTaskInterface()->AddTask([a_from]() {
		a_from->Disable();
		a_from->Enable(false);
	});
    /*a_from->Disable();
    a_from->Enable(false);*/

    /*float afX = 100;
    float afY = 100;
    float afZ = 100;
    float afMagnitude = 100;*/
    /*auto args = RE::MakeFunctionArguments(std::move(afX), std::move(afY), std::move(afZ),
    std::move(afMagnitude)); vm->DispatchMethodCall(object, "ApplyHavokImpulse", args, callback);*/
    // Looked up here (wSkeever): https:  // www.nexusmods.com/skyrimspecialedition/mods/73607
    /*SKSE::GetTaskInterface()->AddTask([a_from]() {
        ApplyHavokImpulse(a_from, 0.f, 0.f, 10.f, 5000.f);
    });*/
    //SKSE::GetTaskInterface()->AddTask([a_from]() {
    //    // auto player_ch = RE::PlayerCharacter::GetSingleton();
    //    // player_ch->StartGrabObject();
    //    auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    //    auto policy = vm->GetObjectHandlePolicy();
    //    auto handle = policy->GetHandleForObject(a_from->GetFormType(), a_from);
    //    RE::BSTSmartPointer<RE::BSScript::Object> object;
    //    vm->CreateObject2("ObjectReference", object);
    //    if (!object) logger::warn("Object is null");
    //    vm->BindObject(object, handle, false);
    //    auto args = RE::MakeFunctionArguments(std::move(0.f), std::move(0.f), std::move(1.f), std::move(5.f));
    //    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
    //    if (vm->DispatchMethodCall(object, "ApplyHavokImpulse", args, callback)) logger::trace("FUSRODAH");
    //});
}

float WorldObject::GetDistanceFromPlayer(const RE::TESObjectREFR* ref)
{
    if (!ref) {
		logger::error("Ref is null.");
		return 0;
	}
    const auto player_pos = RE::PlayerCharacter::GetSingleton()->GetPosition();
    const auto ref_pos = ref->GetPosition();
	return player_pos.GetDistance(ref_pos);
}

bool WorldObject::PlayerPickUpObject(RE::TESObjectREFR* item, const Count count, const unsigned int max_try) {
    logger::trace("PickUpItem");

    if (!item) {
        logger::warn("Item is null");
        return false;
    }

    const auto actor = RE::PlayerCharacter::GetSingleton();
    const auto item_bound = item->GetObjectReference();
    const auto item_count = Inventory::GetItemCount(item_bound, actor);
    logger::trace("Item count: {}", item_count);
    unsigned int i = 0;
    if (!item_bound) {
        logger::warn("Item bound is null");
        return false;
    }
    while (i < max_try) {
        logger::trace("Critical: PickUpItem");
        actor->PickUpObject(item, count, false, false);
        logger::trace("Item picked up. Checking if it is in inventory...");
        if (Inventory::GetItemCount(item_bound, actor) > item_count) {
            logger::trace("Item picked up. Took {} extra tries.", i);
            return true;
        } else logger::trace("item count: {}", Inventory::GetItemCount(item_bound, actor));
        i++;
    }

    return false;
}

RefID WorldObject::TryToGetRefIDFromHandle(const RE::ObjectRefHandle& handle) {
    if (handle.get() && handle.get()->GetFormID()) return handle.get()->GetFormID();
    if (handle.native_handle()
        //&& RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(handle.native_handle())
    ) return handle.native_handle();
    return 0;
}

RE::TESObjectREFR* WorldObject::TryToGetRefFromHandle(RE::ObjectRefHandle& handle, unsigned int max_try)
 {
    RE::TESObjectREFR* ref;
    if (const auto handle_ref = RE::TESObjectREFR::LookupByHandle(handle.native_handle())) {
        logger::trace("Handle ref found");
        ref = handle_ref.get();
        return ref;
        /*if (!ref->IsDisabled() && !ref->IsMarkedForDeletion() && !ref->IsDeleted()) {
            return ref;
        }*/
    }
    if (handle.get()) {
        ref = handle.get().get();
        return ref;
        /*if (!ref->IsDisabled() && !ref->IsMarkedForDeletion() && !ref->IsDeleted()) {
            return ref;
        }*/
    }
    if (const auto ref_ = RE::TESForm::LookupByID<RE::TESObjectREFR>(handle.native_handle())) {
        return ref_;
        /*if (!ref_->IsDisabled() && !ref_->IsMarkedForDeletion() && !ref_->IsDeleted()) {
            return ref_;
        }*/
    }
    if (max_try && handle) return TryToGetRefFromHandle(handle, --max_try);
    return nullptr;
}

RE::TESObjectREFR* WorldObject::TryToGetRefInCell(const FormID baseid, const Count count, const float radius)
 {
    const auto player = RE::PlayerCharacter::GetSingleton();
    const auto player_cell = player->GetParentCell();
    if (!player_cell) {
		logger::error("Player cell is null.");
		return nullptr;
	}
    const auto player_pos = player->GetPosition();
    auto& runtimeData = player_cell->GetRuntimeData();
    RE::BSSpinLockGuard locker(runtimeData.spinLock);
    for (const auto& ref : runtimeData.references) {
        if (!ref) continue;
        /*if (ref->IsDisabled()) continue;
        if (ref->IsMarkedForDeletion()) continue;
        if (ref->IsDeleted()) continue;*/
        const auto ref_base = ref->GetBaseObject();
        if (!ref_base) continue;
        const auto ref_baseid = ref_base->GetFormID();
        const auto ref_id = ref->GetFormID();
        const auto ref_pos = ref->GetPosition();
        if (ref_baseid == baseid && ref->extraList.GetCount() == count) {
            // get radius and check if ref is in radius
            if (ref_id < 4278190080) {
                logger::trace("Ref is a placed reference. Continuing search.");
                continue;
            }
            logger::trace("Ref found in cell: {} with id {}", ref_base->GetName(), ref_id);
            if (radius > 0.f) {
                if (player_pos.GetDistance(ref_pos) < radius) return ref.get();
                else logger::trace("Ref is not in radius");
            } else return ref.get();
        }
    }
    return nullptr;
}

std::string String::toLowercase(const std::string& str)
{
    std::string result = str;
    std::ranges::transform(result, result.begin(),
                           [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string String::replaceLineBreaksWithSpace(const std::string& input)
{
    std::string result = input;
    std::ranges::replace(result, '\n', ' ');
    return result;
}

std::string String::trim(const std::string& str)
{
    // Find the first non-whitespace character from the beginning
    const size_t start = str.find_first_not_of(" \t\n\r");

    // If the string is all whitespace, return an empty string
    if (start == std::string::npos) return "";

    // Find the last non-whitespace character from the end
    const size_t end = str.find_last_not_of(" \t\n\r");

    // Return the substring containing the trimmed characters
    return str.substr(start, end - start + 1);
}

bool String::includesWord(const std::string& input, const std::vector<std::string>& strings)
{
    std::string lowerInput = toLowercase(input);
    lowerInput = replaceLineBreaksWithSpace(lowerInput);
    lowerInput = trim(lowerInput);
    lowerInput = " " + lowerInput + " ";  // Add spaces to the beginning and end of the string

    for (const auto& str : strings) {
        std::string lowerStr = str;
        lowerStr = trim(lowerStr);
        lowerStr = " " + lowerStr + " ";  // Add spaces to the beginning and end of the string
        std::ranges::transform(lowerStr, lowerStr.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    
        //logger::trace("lowerInput: {} lowerStr: {}", lowerInput, lowerStr);

        if (lowerInput.find(lowerStr) != std::string::npos) {
            return true;  // The input string includes one of the strings
        }
    }
    return false;  // None of the strings in 'strings' were found in the input string
}

bool xData::UpdateExtras(RE::ExtraDataList* copy_from, RE::ExtraDataList* copy_to) {
    logger::trace("UpdateExtras");
    if (!copy_from || !copy_to) return false;
    // Enchantment
    if (copy_from->HasType(RE::ExtraDataType::kEnchantment)) {
        logger::trace("Enchantment found");
        const auto enchantment =
            skyrim_cast<RE::ExtraEnchantment*>(copy_from->GetByType(RE::ExtraDataType::kEnchantment));
        if (enchantment) {
            RE::ExtraEnchantment* enchantment_fake = RE::BSExtraData::Create<RE::ExtraEnchantment>();
            // log the associated actor value
            logger::trace("Associated actor value: {}", enchantment->enchantment->GetAssociatedSkill());
            Copy::CopyEnchantment(enchantment, enchantment_fake);
            copy_to->Add(enchantment_fake);
        } else return false;
    }
    // Health
    if (copy_from->HasType(RE::ExtraDataType::kHealth)) {
        logger::trace("Health found");
        if (const auto health = skyrim_cast<RE::ExtraHealth*>(copy_from->GetByType(RE::ExtraDataType::kHealth))) {
            RE::ExtraHealth* health_fake = RE::BSExtraData::Create<RE::ExtraHealth>();
            Copy::CopyHealth(health, health_fake);
            copy_to->Add(health_fake);
        } else return false;
    }
    // Rank
    if (copy_from->HasType(RE::ExtraDataType::kRank)) {
        logger::trace("Rank found");
        if (const auto rank = skyrim_cast<RE::ExtraRank*>(copy_from->GetByType(RE::ExtraDataType::kRank))) {
            RE::ExtraRank* rank_fake = RE::BSExtraData::Create<RE::ExtraRank>();
            Copy::CopyRank(rank, rank_fake);
            copy_to->Add(rank_fake);
        } else return false;
    }
    // TimeLeft
    if (copy_from->HasType(RE::ExtraDataType::kTimeLeft)) {
        logger::trace("TimeLeft found");
        if (const auto timeleft = skyrim_cast<RE::ExtraTimeLeft*>(copy_from->GetByType(RE::ExtraDataType::kTimeLeft))) {
            RE::ExtraTimeLeft* timeleft_fake = RE::BSExtraData::Create<RE::ExtraTimeLeft>();
            Copy::CopyTimeLeft(timeleft, timeleft_fake);
            copy_to->Add(timeleft_fake);
        } else return false;
    }
    // Charge
    if (copy_from->HasType(RE::ExtraDataType::kCharge)) {
        logger::trace("Charge found");
        if (const auto charge = skyrim_cast<RE::ExtraCharge*>(copy_from->GetByType(RE::ExtraDataType::kCharge))) {
            RE::ExtraCharge* charge_fake = RE::BSExtraData::Create<RE::ExtraCharge>();
            Copy::CopyCharge(charge, charge_fake);
            copy_to->Add(charge_fake);
        } else return false;
    }
    // Scale
    if (copy_from->HasType(RE::ExtraDataType::kScale)) {
        logger::trace("Scale found");
        if (const auto scale = skyrim_cast<RE::ExtraScale*>(copy_from->GetByType(RE::ExtraDataType::kScale))) {
            RE::ExtraScale* scale_fake = RE::BSExtraData::Create<RE::ExtraScale>();
            Copy::CopyScale(scale, scale_fake);
            copy_to->Add(scale_fake);
        } else return false;
    }
    // UniqueID
    if (copy_from->HasType(RE::ExtraDataType::kUniqueID)) {
        logger::trace("UniqueID found");
        const auto uniqueid = skyrim_cast<RE::ExtraUniqueID*>(copy_from->GetByType(RE::ExtraDataType::kUniqueID));
        if (uniqueid) {
            RE::ExtraUniqueID* uniqueid_fake = RE::BSExtraData::Create<RE::ExtraUniqueID>();
            Copy::CopyUniqueID(uniqueid, uniqueid_fake);
            copy_to->Add(uniqueid_fake);
        } else return false;
    }
    // Poison
    if (copy_from->HasType(RE::ExtraDataType::kPoison)) {
        logger::trace("Poison found");
        if (const auto poison = skyrim_cast<RE::ExtraPoison*>(copy_from->GetByType(RE::ExtraDataType::kPoison))) {
            RE::ExtraPoison* poison_fake = RE::BSExtraData::Create<RE::ExtraPoison>();
            Copy::CopyPoison(poison, poison_fake);
            copy_to->Add(poison_fake);
        } else return false;
    }
    // ObjectHealth
    if (copy_from->HasType(RE::ExtraDataType::kObjectHealth)) {
        logger::trace("ObjectHealth found");
        const auto objhealth =
            skyrim_cast<RE::ExtraObjectHealth*>(copy_from->GetByType(RE::ExtraDataType::kObjectHealth));
        if (objhealth) {
            RE::ExtraObjectHealth* objhealth_fake = RE::BSExtraData::Create<RE::ExtraObjectHealth>();
            Copy::CopyObjectHealth(objhealth, objhealth_fake);
            copy_to->Add(objhealth_fake);
        } else return false;
    }
    // Light
    if (copy_from->HasType(RE::ExtraDataType::kLight)) {
        logger::trace("Light found");
        if (const auto light = skyrim_cast<RE::ExtraLight*>(copy_from->GetByType(RE::ExtraDataType::kLight))) {
            RE::ExtraLight* light_fake = RE::BSExtraData::Create<RE::ExtraLight>();
            Copy::CopyLight(light, light_fake);
            copy_to->Add(light_fake);
        } else return false;
    }
    // Radius
    if (copy_from->HasType(RE::ExtraDataType::kRadius)) {
        logger::trace("Radius found");
        if (const auto radius = skyrim_cast<RE::ExtraRadius*>(copy_from->GetByType(RE::ExtraDataType::kRadius))) {
            RE::ExtraRadius* radius_fake = RE::BSExtraData::Create<RE::ExtraRadius>();
            Copy::CopyRadius(radius, radius_fake);
            copy_to->Add(radius_fake);
        } else return false;
    }
    // Sound (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kSound)) {
        logger::trace("Sound found");
        auto sound = static_cast<RE::ExtraSound*>(copy_from->GetByType(RE::ExtraDataType::kSound));
        if (sound) {
            RE::ExtraSound* sound_fake = RE::BSExtraData::Create<RE::ExtraSound>();
            sound_fake->handle = sound->handle;
            copy_to->Add(sound_fake);
        } else
            RaiseMngrErr("Failed to get radius from copy_from");
    }*/
    // LinkedRef (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kLinkedRef)) {
        logger::trace("LinkedRef found");
        auto linkedref =
            static_cast<RE::ExtraLinkedRef*>(copy_from->GetByType(RE::ExtraDataType::kLinkedRef));
        if (linkedref) {
            RE::ExtraLinkedRef* linkedref_fake = RE::BSExtraData::Create<RE::ExtraLinkedRef>();
            linkedref_fake->linkedRefs = linkedref->linkedRefs;
            copy_to->Add(linkedref_fake);
        } else
            RaiseMngrErr("Failed to get linkedref from copy_from");
    }*/
    // Horse
    if (copy_from->HasType(RE::ExtraDataType::kHorse)) {
        logger::trace("Horse found");
        if (const auto horse = skyrim_cast<RE::ExtraHorse*>(copy_from->GetByType(RE::ExtraDataType::kHorse))) {
            RE::ExtraHorse* horse_fake = RE::BSExtraData::Create<RE::ExtraHorse>();
            Copy::CopyHorse(horse, horse_fake);
            copy_to->Add(horse_fake);
        } else return false;
    }
    // Hotkey
    if (copy_from->HasType(RE::ExtraDataType::kHotkey)) {
        logger::trace("Hotkey found");
        if (const auto hotkey = skyrim_cast<RE::ExtraHotkey*>(copy_from->GetByType(RE::ExtraDataType::kHotkey))) {
            RE::ExtraHotkey* hotkey_fake = RE::BSExtraData::Create<RE::ExtraHotkey>();
            Copy::CopyHotkey(hotkey, hotkey_fake);
            copy_to->Add(hotkey_fake);
        } else return false;
    }
    // Weapon Attack Sound (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kWeaponAttackSound)) {
        logger::trace("WeaponAttackSound found");
        auto weaponattacksound = static_cast<RE::ExtraWeaponAttackSound*>(
            copy_from->GetByType(RE::ExtraDataType::kWeaponAttackSound));
        if (weaponattacksound) {
            RE::ExtraWeaponAttackSound* weaponattacksound_fake =
                RE::BSExtraData::Create<RE::ExtraWeaponAttackSound>();
            weaponattacksound_fake->handle = weaponattacksound->handle;
            copy_to->Add(weaponattacksound_fake);
        } else
            RaiseMngrErr("Failed to get weaponattacksound from copy_from");
    }*/
    // Activate Ref (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kActivateRef)) {
        logger::trace("ActivateRef found");
        auto activateref =
            static_cast<RE::ExtraActivateRef*>(copy_from->GetByType(RE::ExtraDataType::kActivateRef));
        if (activateref) {
            RE::ExtraActivateRef* activateref_fake = RE::BSExtraData::Create<RE::ExtraActivateRef>();
            activateref_fake->parents = activateref->parents;
            activateref_fake->activateFlags = activateref->activateFlags;
        } else
            RaiseMngrErr("Failed to get activateref from copy_from");
    }*/
    // TextDisplayData
    if (copy_from->HasType(RE::ExtraDataType::kTextDisplayData)) {
        logger::trace("TextDisplayData found");
        const auto textdisplaydata =
            skyrim_cast<RE::ExtraTextDisplayData*>(copy_from->GetByType(RE::ExtraDataType::kTextDisplayData));
        if (textdisplaydata) {
            RE::ExtraTextDisplayData* textdisplaydata_fake =
                RE::BSExtraData::Create<RE::ExtraTextDisplayData>();
            Copy::CopyTextDisplayData(textdisplaydata, textdisplaydata_fake);
            copy_to->Add(textdisplaydata_fake);
        } else return false;
    }
    // Soul
    if (copy_from->HasType(RE::ExtraDataType::kSoul)) {
        logger::trace("Soul found");
        if (const auto soul = skyrim_cast<RE::ExtraSoul*>(copy_from->GetByType(RE::ExtraDataType::kSoul))) {
            RE::ExtraSoul* soul_fake = RE::BSExtraData::Create<RE::ExtraSoul>();
            Copy::CopySoul(soul, soul_fake);
            copy_to->Add(soul_fake);
        } else return false;
    }
    // Flags (OK)
    if (copy_from->HasType(RE::ExtraDataType::kFlags)) {
        logger::trace("Flags found");
        if (const auto flags = skyrim_cast<RE::ExtraFlags*>(copy_from->GetByType(RE::ExtraDataType::kFlags))) {
            SKSE::stl::enumeration<RE::ExtraFlags::Flag, std::uint32_t> flags_fake;
            if (flags->flags.all(RE::ExtraFlags::Flag::kBlockActivate))
                flags_fake.set(RE::ExtraFlags::Flag::kBlockActivate);
            if (flags->flags.all(RE::ExtraFlags::Flag::kBlockPlayerActivate))
                flags_fake.set(RE::ExtraFlags::Flag::kBlockPlayerActivate);
            if (flags->flags.all(RE::ExtraFlags::Flag::kBlockLoadEvents))
                flags_fake.set(RE::ExtraFlags::Flag::kBlockLoadEvents);
            if (flags->flags.all(RE::ExtraFlags::Flag::kBlockActivateText))
                flags_fake.set(RE::ExtraFlags::Flag::kBlockActivateText);
            if (flags->flags.all(RE::ExtraFlags::Flag::kPlayerHasTaken))
                flags_fake.set(RE::ExtraFlags::Flag::kPlayerHasTaken);
            // RE::ExtraFlags* flags_fake = RE::BSExtraData::Create<RE::ExtraFlags>();
            // flags_fake->flags = flags->flags;
            // copy_to->Add(flags_fake);
        } else return false;
    }
    // Lock (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kLock)) {
        logger::trace("Lock found");
        auto lock = static_cast<RE::ExtraLock*>(copy_from->GetByType(RE::ExtraDataType::kLock));
        if (lock) {
            RE::ExtraLock* lock_fake = RE::BSExtraData::Create<RE::ExtraLock>();
            lock_fake->lock = lock->lock;
            copy_to->Add(lock_fake);
        } else
            RaiseMngrErr("Failed to get lock from copy_from");
    }*/
    // Teleport (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kTeleport)) {
        logger::trace("Teleport found");
        auto teleport =
            static_cast<RE::ExtraTeleport*>(copy_from->GetByType(RE::ExtraDataType::kTeleport));
        if (teleport) {
            RE::ExtraTeleport* teleport_fake = RE::BSExtraData::Create<RE::ExtraTeleport>();
            teleport_fake->teleportData = teleport->teleportData;
            copy_to->Add(teleport_fake);
        } else
            RaiseMngrErr("Failed to get teleport from copy_from");
    }*/
    // LockList (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kLockList)) {
        logger::trace("LockList found");
        auto locklist =
            static_cast<RE::ExtraLockList*>(copy_from->GetByType(RE::ExtraDataType::kLockList));
        if (locklist) {
            RE::ExtraLockList* locklist_fake = RE::BSExtraData::Create<RE::ExtraLockList>();
            locklist_fake->list = locklist->list;
            copy_to->Add(locklist_fake);
        } else
            RaiseMngrErr("Failed to get locklist from copy_from");
    }*/
    // OutfitItem (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kOutfitItem)) {
        logger::trace("OutfitItem found");
        auto outfititem =
            static_cast<RE::ExtraOutfitItem*>(copy_from->GetByType(RE::ExtraDataType::kOutfitItem));
        if (outfititem) {
            RE::ExtraOutfitItem* outfititem_fake = RE::BSExtraData::Create<RE::ExtraOutfitItem>();
            outfititem_fake->id = outfititem->id;
            copy_to->Add(outfititem_fake);
        } else
            RaiseMngrErr("Failed to get outfititem from copy_from");
    }*/
    // CannotWear (Disabled)
    /*if (copy_from->HasType(RE::ExtraDataType::kCannotWear)) {
        logger::trace("CannotWear found");
        auto cannotwear =
            static_cast<RE::ExtraCannotWear*>(copy_from->GetByType(RE::ExtraDataType::kCannotWear));
        if (cannotwear) {
            RE::ExtraCannotWear* cannotwear_fake = RE::BSExtraData::Create<RE::ExtraCannotWear>();
            copy_to->Add(cannotwear_fake);
        } else
            RaiseMngrErr("Failed to get cannotwear from copy_from");
    }*/
    // Ownership (OK)
    if (copy_from->HasType(RE::ExtraDataType::kOwnership)) {
        logger::trace("Ownership found");
        if (const auto ownership = skyrim_cast<RE::ExtraOwnership*>(copy_from->GetByType(RE::ExtraDataType::kOwnership))) {
            logger::trace("length of fake extradatalist: {}", copy_to->GetCount());
            RE::ExtraOwnership* ownership_fake = RE::BSExtraData::Create<RE::ExtraOwnership>();
            Copy::CopyOwnership(ownership, ownership_fake);
            copy_to->Add(ownership_fake);
            logger::trace("length of fake extradatalist: {}", copy_to->GetCount());
        } else return false;
    }

    return true;
}

void Math::LinAlg::R3::rotateX(RE::NiPoint3& v, float angle)
{
    const float y = v.y * cos(angle) - v.z * sin(angle);
    const float z = v.y * sin(angle) + v.z * cos(angle);
    v.y = y;
    v.z = z;
}

void Math::LinAlg::R3::rotateY(RE::NiPoint3& v, float angle)
{
    const float x = v.x * cos(angle) + v.z * sin(angle);
    const float z = -v.x * sin(angle) + v.z * cos(angle);
    v.x = x;
    v.z = z;
}

void Math::LinAlg::R3::rotateZ(RE::NiPoint3& v, float angle)
{
    const float x = v.x * cos(angle) - v.y * sin(angle);
    const float y = v.x * sin(angle) + v.y * cos(angle);
    v.x = x;
    v.y = y;
}

void Math::LinAlg::R3::rotate(RE::NiPoint3& v, float angleX, float angleY, float angleZ)
{
    rotateX(v, angleX);
	rotateY(v, angleY);
	rotateZ(v, angleZ);
}

bool Inventory::EntryHasXData(const RE::InventoryEntryData* entry) {
    if (entry && entry->extraLists && !entry->extraLists->empty()) return true;
    return false;
}

bool Inventory::HasItemEntry(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner,
                             bool nonzero_entry_check) {
    if (!item) {
        logger::warn("Item is null");
        return 0;
    }
    if (!inventory_owner) {
        logger::warn("Inventory owner is null");
        return 0;
    }
    auto inventory = inventory_owner->GetInventory();
    const auto it = inventory.find(item);
    const bool has_entry = it != inventory.end();
    if (nonzero_entry_check) return has_entry && it->second.first > 0;
    return has_entry;
}

std::int32_t Inventory::GetItemCount(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner) {
    if (HasItemEntry(item, inventory_owner, true)) {
        const auto inventory = inventory_owner->GetInventory();
        const auto it = inventory.find(item);
		return it->second.first;
	}
    return 0;
}

bool Inventory::IsQuestItem(const FormID formid, RE::TESObjectREFR* inv_owner)
{
    const auto inventory = inv_owner->GetInventory();
    const auto item = GetFormByID<RE::TESBoundObject>(formid);
    if (item) {
        if (const auto it = inventory.find(item); it != inventory.end()) {
            if (it->second.second->IsQuestObject()) return true;
        }
    }
    return false;
}

void DynamicForm::copyBookAppearence(RE::TESForm* source, RE::TESForm* target)
{
    const auto* sourceBook = source->As<RE::TESObjectBOOK>();

    auto* targetBook = target->As<RE::TESObjectBOOK>();

    if (sourceBook && targetBook) {
        targetBook->inventoryModel = sourceBook->inventoryModel;
    }
}

void DynamicForm::copyFormArmorModel(RE::TESForm* source, RE::TESForm* target)
{
    const auto* sourceModelBipedForm = source->As<RE::TESObjectARMO>();

    auto* targeteModelBipedForm = target->As<RE::TESObjectARMO>();

    if (sourceModelBipedForm && targeteModelBipedForm) {
        logger::info("armor");

        targeteModelBipedForm->armorAddons = sourceModelBipedForm->armorAddons;
    }
}

void DynamicForm::copyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target)
{
    const auto* sourceModelWeapon = source->As<RE::TESObjectWEAP>();

    auto* targeteModelWeapon = target->As<RE::TESObjectWEAP>();

    if (sourceModelWeapon && targeteModelWeapon) {
        logger::info("weapon");

        targeteModelWeapon->firstPersonModelObject = sourceModelWeapon->firstPersonModelObject;

        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;

        targeteModelWeapon->attackSound2D = sourceModelWeapon->attackSound2D;

        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;

        targeteModelWeapon->attackFailSound = sourceModelWeapon->attackFailSound;

        targeteModelWeapon->idleSound = sourceModelWeapon->idleSound;

        targeteModelWeapon->equipSound = sourceModelWeapon->equipSound;

        targeteModelWeapon->unequipSound = sourceModelWeapon->unequipSound;

        targeteModelWeapon->soundLevel = sourceModelWeapon->soundLevel;
    }
}

void DynamicForm::copyMagicEffect(RE::TESForm* source, RE::TESForm* target)
{
    const auto* sourceEffect = source->As<RE::EffectSetting>();

    auto* targetEffect = target->As<RE::EffectSetting>();

    if (sourceEffect && targetEffect) {
        targetEffect->effectSounds = sourceEffect->effectSounds;

        targetEffect->data.castingArt = sourceEffect->data.castingArt;

        targetEffect->data.light = sourceEffect->data.light;

        targetEffect->data.hitEffectArt = sourceEffect->data.hitEffectArt;

        targetEffect->data.effectShader = sourceEffect->data.effectShader;

        targetEffect->data.hitVisuals = sourceEffect->data.hitVisuals;

        targetEffect->data.enchantShader = sourceEffect->data.enchantShader;

        targetEffect->data.enchantEffectArt = sourceEffect->data.enchantEffectArt;

        targetEffect->data.enchantVisuals = sourceEffect->data.enchantVisuals;

        targetEffect->data.projectileBase = sourceEffect->data.projectileBase;

        targetEffect->data.explosion = sourceEffect->data.explosion;

        targetEffect->data.impactDataSet = sourceEffect->data.impactDataSet;

        targetEffect->data.imageSpaceMod = sourceEffect->data.imageSpaceMod;
    }
}

void DynamicForm::copyAppearence(RE::TESForm* source, RE::TESForm* target)
{
    copyFormArmorModel(source, target);

    copyFormObjectWeaponModel(source, target);

    copyMagicEffect(source, target);

    copyBookAppearence(source, target);

    copyComponent<RE::BGSPickupPutdownSounds>(source, target);

    copyComponent<RE::BGSMenuDisplayObject>(source, target);

    copyComponent<RE::TESModel>(source, target);

    copyComponent<RE::TESBipedModelForm>(source, target);
}

void xData::Copy::CopyEnchantment(const RE::ExtraEnchantment* from, RE::ExtraEnchantment* to)
{
    logger::trace("CopyEnchantment");
	to->enchantment = from->enchantment;
	to->charge = from->charge;
	to->removeOnUnequip = from->removeOnUnequip;
}

void xData::Copy::CopyHealth(const RE::ExtraHealth* from, RE::ExtraHealth* to)
{
    logger::trace("CopyHealth");
    to->health = from->health;
}

void xData::Copy::CopyRank(const RE::ExtraRank* from, RE::ExtraRank* to)
{
    logger::trace("CopyRank");
	to->rank = from->rank;
}

void xData::Copy::CopyTimeLeft(const RE::ExtraTimeLeft* from, RE::ExtraTimeLeft* to)
{
    logger::trace("CopyTimeLeft");
    to->time = from->time;
}

void xData::Copy::CopyCharge(const RE::ExtraCharge* from, RE::ExtraCharge* to)
{
    logger::trace("CopyCharge");
	to->charge = from->charge;
}

void xData::Copy::CopyScale(const RE::ExtraScale* from, RE::ExtraScale* to)
{
    logger::trace("CopyScale");
	to->scale = from->scale;
}

void xData::Copy::CopyUniqueID(const RE::ExtraUniqueID* from, RE::ExtraUniqueID* to)
{
    logger::trace("CopyUniqueID");
    to->baseID = from->baseID;
    to->uniqueID = from->uniqueID;
}

void xData::Copy::CopyPoison(const RE::ExtraPoison* from, RE::ExtraPoison* to)
{
    logger::trace("CopyPoison");
	to->poison = from->poison;
	to->count = from->count;
}

void xData::Copy::CopyObjectHealth(const RE::ExtraObjectHealth* from, RE::ExtraObjectHealth* to)
{
    logger::trace("CopyObjectHealth");
	to->health = from->health;
}

void xData::Copy::CopyLight(const RE::ExtraLight* from, RE::ExtraLight* to)
{
    logger::trace("CopyLight");
    to->lightData = from->lightData;
}

void xData::Copy::CopyRadius(const RE::ExtraRadius* from, RE::ExtraRadius* to)
{
    logger::trace("CopyRadius");
	to->radius = from->radius;
}

void xData::Copy::CopyHorse(const RE::ExtraHorse* from, RE::ExtraHorse* to)
{
    logger::trace("CopyHorse");
    to->horseRef = from->horseRef;
}

void xData::Copy::CopyHotkey(const RE::ExtraHotkey* from, RE::ExtraHotkey* to)
{
    logger::trace("CopyHotkey");
	to->hotkey = from->hotkey;
}

void xData::Copy::CopyTextDisplayData(const RE::ExtraTextDisplayData* from, RE::ExtraTextDisplayData* to)
{
    to->displayName = from->displayName;
    to->displayNameText = from->displayNameText;
    to->ownerQuest = from->ownerQuest;
    to->ownerInstance = from->ownerInstance;
    to->temperFactor = from->temperFactor;
    to->customNameLength = from->customNameLength;
}

void xData::Copy::CopySoul(const RE::ExtraSoul* from, RE::ExtraSoul* to)
{
    logger::trace("CopySoul");
    to->soul = from->soul;
}

void xData::Copy::CopyOwnership(const RE::ExtraOwnership* from, RE::ExtraOwnership* to)
{
    logger::trace("CopyOwnership");
	to->owner = from->owner;
}

RE::TESObjectREFR* Menu::GetContainerFromMenu()
{
    const auto ui = RE::UI::GetSingleton()->GetMenu<RE::ContainerMenu>();
	if (!ui) {
		logger::warn("GetContainerFromMenu: Container menu is null");
		return nullptr;
	}
	auto ui_refid = ui->GetTargetRefHandle();
	if (!ui_refid) {
		logger::warn("GetContainerFromMenu: Container menu reference id is null");
		return nullptr;
	}
    logger::trace("UI Reference id {}", ui_refid);
    if (const auto ui_ref = RE::TESObjectREFR::LookupByHandle(ui_refid)) {
        logger::trace("UI Reference name {}", ui_ref->GetDisplayFullName());
        return ui_ref.get();
    }
    return nullptr;
}

RE::TESObjectREFR* Menu::GetVendorChestFromMenu()
{
    const auto ui = RE::UI::GetSingleton()->GetMenu<RE::BarterMenu>();
	if (!ui) {
		logger::warn("GetVendorChestFromMenu: Barter menu is null");
		return nullptr;
	}
    const auto ui_ref = RE::TESObjectREFR::LookupByHandle(ui->GetTargetRefHandle());
	if (!ui_ref) {
		logger::warn("GetVendorChestFromMenu: Barter menu reference is null");
		return nullptr;
	}
	if (ui_ref->IsPlayerRef()) return nullptr;

	if (const auto barter_actor = ui_ref->GetBaseObject()->As<RE::Actor>()) {
        if (const auto* faction = barter_actor->GetVendorFaction()) {
			if (auto* merchant_chest = faction->vendorData.merchantContainer) {
				logger::trace("Found chest with refid {}", merchant_chest->GetFormID());
				return merchant_chest;
			}
        }
	}

	if (const auto barter_npc = ui_ref->GetBaseObject()->As<RE::TESNPC>()) {
	    for (const auto& faction_rank : barter_npc->factions) {
            if (const auto merchant_chest = faction_rank.faction->vendorData.merchantContainer) {
                logger::trace("Found chest with refid {}", merchant_chest->GetFormID());
                return merchant_chest;
            }
        }
	}

	//auto chest = RE::TESObjectREFR::LookupByHandle(ui->GetTargetRefHandle());

	/*for (size_t i = 0; i < 192; i++) {
		if (ui_ref->extraList.HasType(static_cast<RE::ExtraDataType>(i))) {
			logger::trace("ExtraData type: {:x}", i);
		}
	}*/



    return nullptr;
}

float Math::Round(const float value, const int n)
{
    const float factor = std::pow(10.0f, n);
    return std::round(value * factor) / factor;
}

float Math::Ceil(const float value, const int n)
{
    const float factor = std::pow(10.0f, n);
    return std::ceil(value * factor) / factor;
}
