#include "Hooks.h"

using namespace Hooks;

template <typename MenuType>
void MenuHook<MenuType>::InstallHook(const REL::VariantID& varID) {
    REL::Relocation<std::uintptr_t> vTable(varID);
    _ProcessMessage = vTable.write_vfunc(0x4, &MenuHook<MenuType>::ProcessMessage_Hook);
}


template <typename MenuType>
RE::UI_MESSAGE_RESULTS MenuHook<MenuType>::ProcessMessage_Hook(RE::UIMessage& a_message) {
    if (const std::string_view menuname = MenuType::MENU_NAME; _strcmpi(a_message.menu.c_str(), std::string(menuname).c_str()) == 0) {
        if (const auto msg_type = static_cast<int>(a_message.type.get()); msg_type == 1) {
			logger::info("Menu opened: {}", menuname);
            const auto ui = RE::UI::GetSingleton();
            const auto menu = ui ? ui->GetMenu<MenuType>() : nullptr;
            if (const auto movie = menu ? menu->uiMovie : nullptr)
            {
                RE::GFxValue something;
                RE::GFxValue something2;
                RE::GFxValue something3;
                //something.CreateEmptyMovieClip(&something2,"asdasd");
                /*something.SetArraySize(2);
                something.SetText("asdasd");*/
                //movie->InvokeFmt("_root.QuestJournalFader.Menu_mc.createEmptyMovieClip",&something,"asdasd","-118");
				movie->CreateArray(&something);
				movie->CreateString(&something2, "asdasd");
				movie->CreateString(&something3, "-118");
                something.SetArraySize(2);
				something.SetElement(0, &something2);
				something.SetElement(1, &something3);

                RE::GFxValue empty_clip;
                RE::GFxValue something4;
                RE::GFxValue something5;
				movie->CreateArray(&something4);
				movie->CreateString(&something5, "console.swf");
				something4.SetArraySize(1);
				something4.SetElement(0, &something5);

                bool deneme = movie->Invoke("_root.QuestJournalFader.Menu_mc.createEmptyMovieClip",&empty_clip,&something,2);
				logger::info("Movie created {}", deneme);
				logger::info("Movie created {}", static_cast<int>(empty_clip.GetType()));
                bool deneme2 = movie->Invoke("_root.QuestJournalFader.Menu_mc.asdasd.loadMovie",nullptr,&something4,1);
				logger::info("Movie loaded {}", deneme2);
            }
        }
    }
    return _ProcessMessage(this, a_message);
}

void Hooks::Install(){
    MenuHook<RE::JournalMenu>::InstallHook(RE::VTABLE_JournalMenu[0]);
    //MenuHook<RE::ContainerMenu>::InstallHook(RE::VTABLE_ContainerMenu[0]);
    //MenuHook<RE::BarterMenu>::InstallHook(RE::VTABLE_BarterMenu[0]);
    //MenuHook<RE::CraftingMenu>::InstallHook(RE::VTABLE_CraftingMenu[0]);
    //MenuHook<RE::DialogueMenu>::InstallHook(RE::VTABLE_DialogueMenu[0]);
    //MenuHook<RE::FavoritesMenu>::InstallHook(RE::VTABLE_FavoritesMenu[0]);
	//MenuHook<RE::InventoryMenu>::InstallHook(RE::VTABLE_InventoryMenu[0]);
    //MenuHook<RE::LockpickingMenu>::InstallHook(RE::VTABLE_LockpickingMenu[0]);
    //MenuHook<RE::MagicMenu>::InstallHook(RE::VTABLE_MagicMenu[0]);
    //MenuHook<RE::MapMenu>::InstallHook(RE::VTABLE_MapMenu[0]);

    /*auto& trampoline = SKSE::GetTrampoline();
    trampoline.create(Hooks::trampoline_size);*/
};