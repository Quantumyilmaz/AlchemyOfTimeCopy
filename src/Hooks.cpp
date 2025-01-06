#include "Hooks.h"

using namespace Hooks;

template <typename MenuType>
void MenuHook<MenuType>::InstallHook(const REL::VariantID& varID, Manager* mngr) {
    REL::Relocation<std::uintptr_t> vTable(varID);
    _ProcessMessage = vTable.write_vfunc(0x4, &MenuHook<MenuType>::ProcessMessage_Hook);
	M = mngr;
}

template <typename MenuType>
RE::UI_MESSAGE_RESULTS MenuHook<MenuType>::ProcessMessage_Hook(RE::UIMessage& a_message) {
    if (const std::string_view menuname = MenuType::MENU_NAME; a_message.menu==menuname) {
        if (const auto msg_type = static_cast<int>(a_message.type.get()); msg_type == 1) {
            if (menuname == RE::FavoritesMenu::MENU_NAME) {
                logger::trace("Favorites menu is open.");
                M->Update(RE::PlayerCharacter::GetSingleton());
            }
            else if (menuname == RE::InventoryMenu::MENU_NAME) {
                logger::trace("Inventory menu is open.");
                M->Update(RE::PlayerCharacter::GetSingleton());
            }
            else if (menuname == RE::BarterMenu::MENU_NAME){
                logger::trace("Barter menu is open.");
                M->Update(RE::PlayerCharacter::GetSingleton());
                //if (const auto vendor_chest = Menu::GetVendorChestFromMenu()) {
                //    M->Update(vendor_chest);
                //} else logger ::error("Could not get vendor chest.");
            } 
            else if (menuname == RE::ContainerMenu::MENU_NAME) {
                logger::trace("Container menu is open.");
                if (const auto container = Menu::GetContainerFromMenu()) {
                    M->Update(RE::PlayerCharacter::GetSingleton());
                    M->Update(container);
                } else logger::error("Could not get container.");
            }
        }
    }
    return _ProcessMessage(this, a_message);
}

void Hooks::Install(Manager* mngr){
    MenuHook<RE::ContainerMenu>::InstallHook(RE::VTABLE_ContainerMenu[0],mngr);
    MenuHook<RE::BarterMenu>::InstallHook(RE::VTABLE_BarterMenu[0],mngr);
    MenuHook<RE::FavoritesMenu>::InstallHook(RE::VTABLE_FavoritesMenu[0],mngr);
	MenuHook<RE::InventoryMenu>::InstallHook(RE::VTABLE_InventoryMenu[0],mngr);
};