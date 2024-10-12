#pragma once
#include "Manager.h"

namespace Hooks {

    /*const uint8_t n_hooks = 0;
    const size_t trampoline_size = n_hooks * 14;*/

    void Install(Manager* mngr);

    template <typename MenuType>
    class MenuHook : public MenuType {
        using ProcessMessage_t = decltype(&MenuType::ProcessMessage);
        static inline REL::Relocation<ProcessMessage_t> _ProcessMessage;
        RE::UI_MESSAGE_RESULTS ProcessMessage_Hook(RE::UIMessage& a_message);
        static inline Manager* M = nullptr;
    public:
        static void InstallHook(REL::VariantID varID, Manager* mngr);
    };
};