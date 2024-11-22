#include "Events.h"

void OurEventSink::HandleWO(RE::TESObjectREFR* ref) const
{
    if (!ref) return;
    M->HandleDynamicWO(ref);
    if (!Settings::world_objects_evolve) return;
    if (ref->IsDisabled() || ref->IsDeleted() || ref->IsMarkedForDeletion()) return;
    if (ref->IsActivationBlocked()) return;
    //if (ref->extraList.GetOwner() && !ref->extraList.GetOwner()->IsPlayer()) return;
    if (RE::PlayerCharacter::GetSingleton()->WouldBeStealing(ref)) return;
    if (!Settings::IsItem(ref)) return;
    if (ref->extraList.HasType(RE::ExtraDataType::kStartingPosition)) {
        if (const auto starting_pos = ref->extraList.GetByType<RE::ExtraStartingPosition>(); starting_pos->location) {
            /*logger::trace("has location.");
            logger::trace("Location: {}", starting_pos->location->GetName());
            logger::trace("Location: {}", starting_pos->location->GetFullName());*/
            return;
        }
        /*logger::trace("Position: {}", starting_pos->startPosition.pos.x);
        logger::trace("Position: {}", starting_pos->startPosition.pos.y);
        logger::trace("Position: {}", starting_pos->startPosition.pos.z);*/
    }

	logger::trace("Handle WO: Calling Update.");
	M->Update(ref);
}

void OurEventSink::HandleWOsInCell() const
{
	logger::trace("HandleWOsInCell: Calling Update.");
    const auto* player = RE::PlayerCharacter::GetSingleton();
    //M->Update(player);
    const auto player_cell = player->GetParentCell();
	if (!player_cell) return;
	player_cell->ForEachReference([this](RE::TESObjectREFR* arg) {
		if (!arg) return RE::BSContainer::ForEachResult::kContinue;
        if (arg->HasContainer()) return RE::BSContainer::ForEachResult::kContinue;
        this->HandleWO(arg); 
		return RE::BSContainer::ForEachResult::kContinue;
    });
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!M->listen_equip.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->actor->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;
    if (!Settings::IsItem(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
    if (!event->equipped) {
        logger::trace("Item unequipped: {}", event->baseObject);
		return RE::BSEventNotifyControl::kContinue;
    }
	if (const auto temp_form = RE::TESForm::LookupByID(event->baseObject); temp_form && temp_form->Is(RE::FormType::AlchemyItem)) {
		logger::trace("Item equipped: Alchemy item.");
		return RE::BSEventNotifyControl::kContinue;
	}
         

	logger::trace("Item equipped: Calling Update.");
	M->Update(RE::PlayerCharacter::GetSingleton());

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->objectActivated) return RE::BSEventNotifyControl::kContinue;
    if (event->objectActivated == RE::PlayerCharacter::GetSingleton()->GetGrabbedRef()) return RE::BSEventNotifyControl::kContinue;
    if (event->objectActivated->IsActivationBlocked()) return RE::BSEventNotifyControl::kContinue;

    if (!M->RefIsRegistered(event->objectActivated->GetFormID())) return RE::BSEventNotifyControl::kContinue;
	if (event->objectActivated->HasContainer()) return RE::BSEventNotifyControl::kContinue;
        
    /*if (M->po3_use_or_take.load()) {
        if (auto base = event->objectActivated->GetBaseObject()) {
            RE::BSString str;
            base->GetActivateText(RE::PlayerCharacter::GetSingleton(), str);
            if (String::includesWord(str.c_str(), {"Eat","Drink"})) activate_eat = true;
        }
    }*/

	if (event->actionRef && event->actionRef->IsPlayerRef()) {
        picked_up_refid = event->objectActivated->GetFormID();
        logger::trace("Picked up: {}, count: {}", picked_up_refid, event->objectActivated->extraList.GetCount());
	}

    M->SwapWithStage(event->objectActivated.get());

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const SKSE::CrosshairRefEvent* event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->crosshairRef) return RE::BSEventNotifyControl::kContinue;

	//if (!M->RefIsRegistered(event->crosshairRef->GetFormID())) return RE::BSEventNotifyControl::kContinue;

	//if (event->crosshairRef->HasContainer()) M->Update(event->crosshairRef.get());
    /*else HandleWO(event->crosshairRef.get());*/

	const auto curr_time = RE::Calendar::GetSingleton()->GetHoursPassed();
	if (last_crosshair_ref_update.first == event->crosshairRef->GetFormID()) {
		if (curr_time - last_crosshair_ref_update.second < min_last_crosshair_update_time) return RE::BSEventNotifyControl::kContinue;
    }

    if (!event->crosshairRef->HasContainer()) HandleWO(event->crosshairRef.get());
	else if (M->RefIsRegistered(event->crosshairRef->GetFormID())) M->Update(event->crosshairRef.get());

	last_crosshair_ref_update = { event->crosshairRef->GetFormID(), curr_time};
        
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESFurnitureEvent* event, RE::BSTEventSource<RE::TESFurnitureEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->actor->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;
    if (furniture_entered && event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter)
        return RE::BSEventNotifyControl::kContinue;
    if (!furniture_entered && event->type == RE::TESFurnitureEvent::FurnitureEventType::kExit)
        return RE::BSEventNotifyControl::kContinue;
    if (event->targetFurniture->GetBaseObject()->formType.underlying() != 40)
        return RE::BSEventNotifyControl::kContinue;

    const auto bench = event->targetFurniture->GetBaseObject()->As<RE::TESFurniture>();
    if (!bench) return RE::BSEventNotifyControl::kContinue;
    auto bench_type = static_cast<std::uint8_t>(bench->workBenchData.benchType.get());
    logger::trace("Furniture event: {}", bench_type);

    //if (bench_type != 2 && bench_type != 3 && bench_type != 7) return RE::BSEventNotifyControl::kContinue;

        
    if (!Settings::qform_bench_map.contains(bench_type)) return RE::BSEventNotifyControl::kContinue;

    if (event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
        logger::trace("Furniture event: Enter {}", event->targetFurniture->GetName());
        furniture_entered = true;
        furniture = event->targetFurniture;
        M->HandleCraftingEnter(bench_type);
    } else if (event->type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
        logger::trace("Furniture event: Exit {}", event->targetFurniture->GetName());
        if (event->targetFurniture == furniture) {
            M->HandleCraftingExit();
            furniture_entered = false;
            furniture = nullptr;
        }
    } else logger::info("Furniture event: Unknown");

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESContainerChangedEvent* event, RE::BSTEventSource<RE::TESContainerChangedEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!M->listen_container_change.load()) return RE::BSEventNotifyControl::kContinue;
    if (furniture_entered && event->newContainer!=player_refid) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->itemCount) return RE::BSEventNotifyControl::kContinue;
    if (!event->baseObj) return RE::BSEventNotifyControl::kContinue;
    if (event->oldContainer==event->newContainer) return RE::BSEventNotifyControl::kContinue;

    auto reference_ = event->reference;
    const auto item = RE::TESForm::LookupByID(event->baseObj);
	auto from_ref = event->oldContainer ? RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(event->oldContainer) : WorldObject::TryToGetRefFromHandle(reference_);
	auto to_ref = event->newContainer ? RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(event->newContainer) : WorldObject::TryToGetRefFromHandle(reference_);
	if (!from_ref) {
		if (RE::UI::GetSingleton()->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
			from_ref = Menu::GetVendorChestFromMenu();
		}
		else if (picked_up_refid) {
		    logger::info("Using picked up refid: {}", picked_up_refid);
			from_ref = RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(picked_up_refid);
			picked_up_refid = 0;
		}
	}
    if (!to_ref){
		if (RE::UI::GetSingleton()->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
			to_ref = Menu::GetVendorChestFromMenu();
        }
		//else to_ref = WorldObject::TryToGetRefInCell(event->baseObj,event->itemCount);
    }

	logger::trace("Container change event: Calling Update.");
	M->Update(from_ref, to_ref, item, event->itemCount);

	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESSleepStopEvent*, RE::BSTEventSource<RE::TESSleepStopEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    logger::trace("Sleep stop event.");
    HandleWOsInCell();
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESWaitStopEvent*, RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    logger::trace("Wait stop event.");
    HandleWOsInCell();
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*)
{
    if (!listen_cellchange.load()) return RE::BSEventNotifyControl::kContinue;
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    auto eventActorHandle = a_event->actor;
    auto eventActorPtr = eventActorHandle ? eventActorHandle.get() : nullptr;
    auto eventActor = eventActorPtr ? eventActorPtr.get() : nullptr;
    if (!eventActor) return RE::BSEventNotifyControl::kContinue;

    if (eventActor != RE::PlayerCharacter::GetSingleton()) return RE::BSEventNotifyControl::kContinue;

    auto cellID = a_event->cellID;
    auto* cellForm = cellID ? RE::TESForm::LookupByID(cellID) : nullptr;
    auto* cell = cellForm ? cellForm->As<RE::TESObjectCELL>() : nullptr;
    if (!cell) return RE::BSEventNotifyControl::kContinue;

    if (a_event->flags.any(RE::BGSActorCellEvent::CellFlag::kEnter)) {
        logger::trace("Player entered cell: {}", cell->GetName());
		listen_cellchange.store(false);
        M->ClearWOUpdateQueue();
        HandleWOsInCell();
		listen_cellchange.store(true);
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
{
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    if (!a_event->formID) return RE::BSEventNotifyControl::kContinue;
	M->HandleFormDelete(a_event->formID);
    return RE::BSEventNotifyControl::kContinue;
}