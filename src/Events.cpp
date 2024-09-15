#include "Events.h"

void OurEventSink::RefreshMenu(const RE::BSFixedString& menuName, RE::TESObjectREFR* inventory)
{
	setListenMenu(false);
    /*if (ui && !ui->IsMenuOpen(menuname)) {
        logger::trace("Menu is not open.");
        setListenMenu(true);
        return;
    }*/
    logger::trace("Refreshing menu: {}", menuName.c_str());
    const auto menuname = menuName.c_str();
    if (!inventory) inventory = RE::PlayerCharacter::GetSingleton();
    if (menuname == RE::FavoritesMenu::MENU_NAME) {
        if (const auto queue = RE::UIMessageQueue::GetSingleton(); queue && ui && ui->IsMenuOpen(menuname)) {
            queue->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kHide, nullptr);
            queue->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kShow, nullptr);
        }
    }
    else if (menuname == RE::InventoryMenu::MENU_NAME){
        Menu::RefreshItemList<RE::InventoryMenu>(inventory);
        Menu::UpdateItemList<RE::InventoryMenu>();
    } 
	else if (menuname == RE::ContainerMenu::MENU_NAME){
        Menu::RefreshItemList<RE::ContainerMenu>(inventory);
        Menu::UpdateItemList<RE::ContainerMenu>();
	} else if (menuname == RE::BarterMenu::MENU_NAME) {
        Menu::RefreshItemList<RE::BarterMenu>(inventory);
        Menu::UpdateItemList<RE::BarterMenu>();
	}
        
    setListenMenu(true);
}

void OurEventSink::HandleWO(RE::TESObjectREFR* ref) const
{
    if (!world_objects_evolve) return;
    if (!ref) return;
    if (ref->IsDisabled() || ref->IsDeleted() || ref->IsMarkedForDeletion()) return;
    if (ref->IsActivationBlocked()) return;
    if (ref->extraList.GetOwner() && !ref->extraList.GetOwner()->IsPlayer()) return;
    if (auto ref_base = ref->GetObjectReference()) /*logger::trace("HandleWO: {}", ref_base->GetName())*/
        ;
    else return;
    if (!Settings::IsItem(ref)) return;

    if (ref->extraList.HasType(RE::ExtraDataType::kStartingPosition)) {
        logger::trace("has Starting position.");
        auto starting_pos = ref->extraList.GetByType<RE::ExtraStartingPosition>();
        if (starting_pos->location) {
            logger::trace("has location.");
            logger::trace("Location: {}", starting_pos->location->GetName());
            logger::trace("Location: {}", starting_pos->location->GetFullName());
            return;
        }
        /*logger::trace("Position: {}", starting_pos->startPosition.pos.x);
        logger::trace("Position: {}", starting_pos->startPosition.pos.y);
        logger::trace("Position: {}", starting_pos->startPosition.pos.z);*/
    }

    if (!M->RefIsRegistered(ref->GetFormID())) {
        logger::trace("Item not registered.");
        if (!M->RegisterAndGo(ref)) {
#ifndef NDEBUG
            logger::warn("Failed to register item.");
#endif  // !NDEBUG
        }
    } else {
        logger::trace("Item registered.");
        M->UpdateStages(ref);
        /*logger::trace("Refreshing menu.");
        Menu::RefreshMenu(RE::InventoryMenu::MENU_NAME);
        logger::trace("Refreshed menu.");*/
    }
}

void OurEventSink::HandleWOsInCell()
{
    if (!world_objects_evolve) return;
    logger::trace("Handling world objects in cell.");
    M->UpdateStages(player_refid);
    WorldObject::ForEachRefInCell(
        [this](RE::TESObjectREFR* arg) { this->HandleWO(arg); });
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

    const auto q_formtype = Settings::GetQFormType(event->baseObject);
    logger::trace("equipped QFormType: {}", q_formtype);
    // only for tracking consumed items
    if (Vector::HasElement<std::string>(Settings::consumableQFORMS, q_formtype)) {
        consume_equipped_id = event->baseObject;
        logger::trace("consume equipped: {}", consume_equipped_id);
        consume_equipped_t = RE::Calendar::GetSingleton()->GetHoursPassed();
        logger::trace("consume equipped time: {}", consume_equipped_t);
    }
    if (Vector::HasElement<std::string>(Settings::updateonequipQFORMS, q_formtype)) {
        logger::trace("Item equipped: {}", event->baseObject);
        M->UpdateStages(player_refid);
    }

        
    // if (event->equipped) {
	//     logger::trace("Item {} was equipped. equipped: {}", event->baseObject);
    // } else {
    //     logger::trace("Item {} was unequipped. equipped: {}", event->baseObject);
    // }
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->objectActivated) return RE::BSEventNotifyControl::kContinue;
    if (event->objectActivated == RE::PlayerCharacter::GetSingleton()->GetGrabbedRef()) return RE::BSEventNotifyControl::kContinue;
    if (event->objectActivated->IsActivationBlocked()) return RE::BSEventNotifyControl::kContinue;

    if (!Settings::IsItem(event->objectActivated.get())) return RE::BSEventNotifyControl::kContinue;
        
    if (!event->actionRef->IsPlayerRef()) {
        logger::trace("Object activated: {} by {}", event->objectActivated->GetName(), event->actionRef->GetName());
        if (M->RefIsRegistered(event->objectActivated.get()->GetFormID()))
            M->SwapWithStage(event->objectActivated.get());
        return RE::BSEventNotifyControl::kContinue;
    }
        
    if (M->po3_use_or_take.load()) {
        if (auto base = event->objectActivated->GetBaseObject()) {
            RE::BSString str;
            base->GetActivateText(RE::PlayerCharacter::GetSingleton(), str);
            if (String::includesWord(str.c_str(), {"Eat","Drink"})) activate_eat = true;
        }
    }
            
    picked_up_time = RE::Calendar::GetSingleton()->GetHoursPassed();
    picked_up_refid = event->objectActivated->GetFormID();
    logger::trace("Picked up: {} at time {}, count: {}", picked_up_refid, picked_up_time, event->objectActivated->extraList.GetCount());

    //M->SwapWithStage(event->objectActivated.get());
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const SKSE::CrosshairRefEvent* event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->crosshairRef) return RE::BSEventNotifyControl::kContinue;
    if (!M->listen_crosshair.load()) return RE::BSEventNotifyControl::kContinue;


    if (M->IsExternalContainer(event->crosshairRef.get())) M->UpdateStages(event->crosshairRef.get());

    HandleWO(event->crosshairRef.get());
        
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OurEventSink::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
    if (block_eventsinks.load()) return RE::BSEventNotifyControl::kContinue;
    if (!event) return RE::BSEventNotifyControl::kContinue;
    if (!event->opening) return RE::BSEventNotifyControl::kContinue;
    if (!getListenMenu()) return RE::BSEventNotifyControl::kContinue;

    if (!ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::BarterMenu::MENU_NAME)) return RE::BSEventNotifyControl::kContinue;

    auto menuname = event->menuName.c_str();
    // return if menu is not favorite menu, container menu, barter menu or inventory menu
    if (menuname == RE::FavoritesMenu::MENU_NAME) {
        logger::trace("Favorites menu is open.");
        if (M->UpdateStages(player_refid)) RefreshMenu(event->menuName, nullptr);
        logger::trace("Spoilage updated.");
        return RE::BSEventNotifyControl::kContinue;
    }
    else if (menuname == RE::InventoryMenu::MENU_NAME) {
        logger::trace("Inventory menu is open.");
        if (M->UpdateStages(player_refid)) RefreshMenu(event->menuName, nullptr);
        logger::trace("Spoilage updated.");
        return RE::BSEventNotifyControl::kContinue;
    }
    else if (menuname == RE::BarterMenu::MENU_NAME){
        logger::trace("Barter menu is open.");
        const auto player_updated = M->UpdateStages(player_refid);
        bool vendor_updated = false;
        if (const auto vendor_chest = Menu::GetVendorChestFromMenu()) {
            vendor_updated = M->UpdateStages(vendor_chest->GetFormID());
        } else logger ::error("Could not get vendor chest.");
        if (player_updated || vendor_updated) RefreshMenu(event->menuName, nullptr);
        return RE::BSEventNotifyControl::kContinue;
    } else if (menuname == RE::ContainerMenu::MENU_NAME) {
        logger::trace("Container menu is open.");
        if (auto container = Menu::GetContainerFromMenu()) {
            if (M->UpdateStages(player_refid) || M->UpdateStages(container->GetFormID())) {
                RefreshMenu(event->menuName, container);
                RefreshMenu(event->menuName, nullptr);
            }
        } else logger::error("Could not get container.");
    }
        
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


    auto bench = event->targetFurniture->GetBaseObject()->As<RE::TESFurniture>();
    if (!bench) return RE::BSEventNotifyControl::kContinue;
    auto bench_type = static_cast<std::uint8_t>(bench->workBenchData.benchType.get());
    logger::trace("Furniture event: {}", bench_type);

    //if (bench_type != 2 && bench_type != 3 && bench_type != 7) return RE::BSEventNotifyControl::kContinue;

        
    if (!Settings::qform_bench_map.contains(bench_type)) return RE::BSEventNotifyControl::kContinue;

    if (event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
        logger::trace("Furniture event: Enter {}", event->targetFurniture->GetName());
        furniture_entered = true;
        furniture = event->targetFurniture;
        M->HandleCraftingEnter(static_cast<unsigned int>(bench_type));
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


    if (!Settings::IsItem(event->baseObj)) {
        logger::trace("Not an item.");
        if (event->oldContainer == player_refid || event->newContainer == player_refid) {
            M->UpdateStages(event->oldContainer);
            M->UpdateStages(event->newContainer);
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    if (event->oldContainer != player_refid && event->newContainer != player_refid && event->reference &&
        M->RefIsRegistered(WorldObject::TryToGetRefIDFromHandle(event->reference)) &&
        event->newContainer) {
        auto external_ref = RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(event->newContainer);
        if (external_ref && external_ref->HasContainer()) {
            M->HandlePickUp(event->baseObj, event->itemCount, event->reference.native_handle(),false,external_ref);
        }
		else logger::trace("ExternalRef not found.");
    }
        
    if (event->oldContainer != player_refid && event->newContainer != player_refid)
        return RE::BSEventNotifyControl::kContinue;
        
    logger::trace("Container change event.");
    //logger::trace("IsStage: {}", M->IsStage(event->baseObj));

    // to player inventory <-
    if (event->newContainer == player_refid) {
        // check if quest item
        if (Inventory::IsQuestItem(event->baseObj,RE::PlayerCharacter::GetSingleton())) {
            logger::trace("Quest item entered player inventory.");
            return RE::BSEventNotifyControl::kContinue;
        }
        logger::trace("Item entered player inventory.");
        if (!event->oldContainer) {
            if (RE::UI::GetSingleton()->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
                logger::trace("Bought from null old container.");
                if (auto vendor_chest = Menu::GetVendorChestFromMenu()) {
                    if (M->HandleBuy(event->baseObj, event->itemCount, vendor_chest->GetFormID())) {
                        RefreshMenu(RE::BarterMenu::MENU_NAME, nullptr);
                        RefreshMenu(RE::BarterMenu::MENU_NAME, vendor_chest);
                    }
				}
				else {
					logger::error("Could not get vendor chest");
#ifndef NDEBUG
					MsgBoxesNotifs::InGame::CustomMsg("Could not get vendor chest.");
#endif  // !NDEBUG
                }
            }
            else { // demek ki world object ya da gaipten geldi
                auto reference_ = event->reference;
                logger::trace("Reference: {}", reference_.native_handle());
                auto ref_id = WorldObject::TryToGetRefIDFromHandle(reference_);
                if (!ref_id) {
                    logger::trace("Could not find reference");
                    ref_id = picked_up_time > 0 ? picked_up_refid : 0;
                    if (!ref_id) {
                        logger::warn("Could not find reference with stored pickedup RefID {}", picked_up_refid);
                        logger::trace("Registering it...");
                        if (!M->RegisterAndGo(event->baseObj, event->itemCount, player_refid)) {
#ifndef NDEBUG
                            logger::warn("Failed to register item.");
#endif  // !NDEBUG
                        }
                        return RE::BSEventNotifyControl::kContinue;
                    }
                    else if (std::abs(picked_up_time - RE::Calendar::GetSingleton()->GetHoursPassed()) > 0.001f) {
                        logger::warn("Picked up time: {}, calendar time: {}. Was it a book?", picked_up_time, RE::Calendar::GetSingleton()->GetHoursPassed());
                    }
                }
                logger::trace("Reference found: {}", ref_id);
                picked_up_refid = 0;
                picked_up_time = 0;
                M->HandlePickUp(event->baseObj, event->itemCount, ref_id, activate_eat);
                activate_eat = false;
            }
        }
        else if (M->IsExternalContainer(event->baseObj,event->oldContainer)) {
            logger::trace("from External container to player inventory.");
            if (M->UnLinkExternalContainer(event->baseObj,event->itemCount,event->oldContainer)){
                if (auto temp_container =
                        RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(event->oldContainer)) {
                    RefreshMenu(RE::ContainerMenu::MENU_NAME, temp_container);
                }
                RefreshMenu(RE::ContainerMenu::MENU_NAME, nullptr);
            }
        }
        // NPC: you dropped this...
        else if (auto ref_id__ = WorldObject::TryToGetRefIDFromHandle(event->reference)) {
            logger::info("NPC: you dropped this...Reference handle refid: {}", ref_id__);
            // bi sekilde yukarda external olarak registerlanmadiysa... ya da genel:
            M->HandlePickUp(event->baseObj, event->itemCount, ref_id__, false);
        }
        else {
			// old container null deil ve registered deil
            logger::trace("Old container not null and not registered.");
            if (!M->RegisterAndGo(event->baseObj, event->itemCount, player_refid)) {
#ifndef NDEBUG
                logger::warn("Failed to register item.");
#endif  // !NDEBUG
			}
        }
    }

    // from player inventory ->
    if (event->oldContainer == player_refid) {
        // a fake container left player inventory
        logger::trace("Item left player inventory.");
        // drop event
        if (!event->newContainer) {
            logger::trace("Dropped.");
			M->listen_crosshair.store(false);
            auto reference_ = event->reference;
            logger::trace("Reference: {}", reference_.native_handle());
            RE::TESObjectREFR* ref = WorldObject::TryToGetRefFromHandle(reference_);
            if (ref) logger::trace("Dropped ref name: {}", ref->GetBaseObject()->GetName());
            if (!ref) {
                /*logger::info("Iterating through all references in the cell.");
                ref = WorldObject::TryToGetRefInCell(event->baseObj,
                                                                                    event->itemCount);*/

                const auto curr_time = RE::Calendar::GetSingleton()->GetHoursPassed();
                if (event->baseObj == consume_equipped_id && 
                    event->itemCount==1 &&
                    std::abs(curr_time - consume_equipped_t) < 0.001f) {
                    M->HandleConsume(event->baseObj);
                    consume_equipped_id = 0;
                    consume_equipped_t = 0;
                } else {
                    // iterate through all objects in the cell................
                    logger::info("Iterating through all references in the cell.");
                    ref = WorldObject::TryToGetRefInCell(event->baseObj,event->itemCount);
                }
            } 
            if (ref) {
                if (M->HandleDropCheck(ref)) M->HandleDrop(event->baseObj, event->itemCount, ref);
                else M->HandleConsume(event->baseObj);
            }
            else if (RE::UI::GetSingleton()->IsMenuOpen(RE::BarterMenu::MENU_NAME)){
                if (auto vendor_chest = Menu::GetVendorChestFromMenu()){
                    if (M->LinkExternalContainer(event->baseObj, event->itemCount, event->newContainer)){
                        RefreshMenu(RE::BarterMenu::MENU_NAME, nullptr);
                        RefreshMenu(RE::BarterMenu::MENU_NAME, vendor_chest);
                    }
                } else if (!Settings::disable_warnings) {
                    logger::error("Could not get vendor chest");
					MsgBoxesNotifs::InGame::CustomMsg("Could not get vendor chest.");
                }
            }
            else if (event->baseObj == consume_equipped_id) {
                M->HandleConsume(event->baseObj);
                consume_equipped_id = 0;
                consume_equipped_t = 0;
            } 
            else logger::warn("Ref not found at HandleDrop! Hopefully due to consume.");
			M->listen_crosshair.store(true);
        }
        // Barter transfer
        else if (RE::UI::GetSingleton()->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
            logger::info("Sold container.");
            if (M->LinkExternalContainer(event->baseObj, event->itemCount, event->newContainer)) {
                RefreshMenu(RE::BarterMenu::MENU_NAME, nullptr);
                if (auto _vendor = RE::TESForm::LookupByID<RE::TESObjectREFR>(event->newContainer)) {
					RefreshMenu(RE::BarterMenu::MENU_NAME, _vendor);
				}
            }
        }
        // container transfer
        else if (RE::UI::GetSingleton()->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
            logger::trace("Container menu is open.");
            if (M->LinkExternalContainer(event->baseObj,event->itemCount,event->newContainer)){
                auto temp_container = RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(event->newContainer);
                RefreshMenu(RE::ContainerMenu::MENU_NAME, temp_container);
                RefreshMenu(RE::ContainerMenu::MENU_NAME, nullptr);
            }
        }
        else {
            logger::warn("Item got removed from player inventory due to unknown reason.");
#ifndef NDEBUG
            MsgBoxesNotifs::InGame::CustomMsg("Item got removed from player inventory due to unknown reason.");
#endif  // !NDEBUG
            // remove from one of the instances
            M->HandleConsume(event->baseObj);
        }
    }
        
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
    if (!getListenCellChange()) return RE::BSEventNotifyControl::kContinue;
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
        setListenCellChange(false);
        M->ClearWOUpdateQueue();
        HandleWOsInCell();
        setListenCellChange(true);
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
