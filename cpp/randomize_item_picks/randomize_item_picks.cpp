#include "ameboid.hpp"
#include "types/glaiel.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/portal.hpp"
#include "utilities/time_profiling.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <random>
#include <unordered_map>

#include "SDL3/SDL.h"

// Exposes a keyboard shortcut to pick random items (R) during equipment drafts.
//
// polymeric 2026

struct PrivateState {
    std::mt19937 prng{std::random_device{}()};

    std::vector<MsvcFuncNoAlloc<void *, void(void)> *> invoke_queue_once_per_update_frame;

    bool block_calls_to_InventoryItemBox__click__lambda_1__Do_call_posttrampoline = false;
};

static PrivateState P;

MAKE_DPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

void on_update_frame() {
    if(!P.invoke_queue_once_per_update_frame.empty()) {
        MAKE_PROFILER_SCOPE(sct, "on_update_frame");
        MAKE_PROFILER_CHECKPOINT(cht, "on_update_frame");
        // get the MewDirector
        MewDirector *p_mewdirector = get_p_mewdirector_singleton();
        PROFILER_CHECKPOINT_CHECK(cht, "get_p_mewdirector_singleton");
        if(p_mewdirector == nullptr) {
            return;
        }

        // get the PauseMenu scene
        Scene *p_pausemenu_scene = nullptr;
        for(auto p_scene : p_mewdirector->director->scenes) {
            if(p_scene->name.as_native_string_view() == "PauseMenu") {
                p_pausemenu_scene = p_scene;
            }
            if(p_pausemenu_scene != nullptr) {
                break;
            }
        }
        PROFILER_CHECKPOINT_CHECK(cht, "p_pausemenu_scene");
        if(p_pausemenu_scene == nullptr) {
            return;
        }

        // get the PauseMenu from the PauseMenu scene
        Component *p_pausemenu = nullptr;
        for(auto p_component : *p_pausemenu_scene->ComponentLists) {
            MsvcReleaseModeXString type_name = {};
            p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
            if(type_name.as_native_string_view() == "PauseMenu") {
                type_name.destroy();
                p_pausemenu = p_component;
                break;
            }
            type_name.destroy();
        }
        PROFILER_CHECKPOINT_CHECK(cht, "p_pausemenu");
        // if the pause menu is loaded, don't continue to avoid a crash
        if(p_pausemenu != nullptr) {
            return;
        }

        auto func = P.invoke_queue_once_per_update_frame.back();
        P.block_calls_to_InventoryItemBox__click__lambda_1__Do_call_posttrampoline = false;
        func->vtable->_Do_call(func);
        P.invoke_queue_once_per_update_frame.pop_back();

        P.block_calls_to_InventoryItemBox__click__lambda_1__Do_call_posttrampoline = !P.invoke_queue_once_per_update_frame.empty();
        PROFILER_CHECKPOINT_CHECK(cht, "end");
    }
}

void shuffle_items_and_schedule_invokes() {
    MAKE_PROFILER_SCOPE(sct, "shuffle_items_and_schedule_invokes");
    MAKE_PROFILER_CHECKPOINT(cht, "shuffle_items_and_schedule_invokes");
    // Only queue button pushes if the queue is empty
    if(P.invoke_queue_once_per_update_frame.empty()) {
        // get the MewDirector
        MewDirector *p_mewdirector = get_p_mewdirector_singleton();
        PROFILER_CHECKPOINT_CHECK(cht, "get_p_mewdirector_singleton");
        if(p_mewdirector == nullptr) {
            return;
        }

        // get the Shared and StorageItems scenes
        Scene *p_shared_scene = nullptr;
        Scene *p_storageitems_scene = nullptr;
        for(auto p_scene : p_mewdirector->director->scenes) {
            if(p_scene->name.as_native_string_view() == "Shared") {
                p_shared_scene = p_scene;
            } else if(p_scene->name.as_native_string_view() == "StorageItems") {
                p_storageitems_scene = p_scene;
            } else if(p_scene->name.as_native_string_view() == "YesNoPrompt") {
                // return if there is a YesNoPrompt to avoid crashing the game
                return;
            }
        }
        PROFILER_CHECKPOINT_CHECK(cht, "p_shared_scene/p_storageitems_scene");
        if(p_shared_scene == nullptr || p_storageitems_scene == nullptr) {
            return;
        }

        // get the Inventory from the Shared scene
        Inventory *p_inventory = nullptr;
        for(auto p_component : *p_shared_scene->ComponentLists) {
            MsvcReleaseModeXString type_name = {};
            p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
            if(type_name.as_native_string_view() == "Inventory") {
                type_name.destroy();
                p_inventory = static_cast<Inventory *>(p_component);
                break;
            }
            type_name.destroy();
        }
        PROFILER_CHECKPOINT_CHECK(cht, "p_inventory");
        if(p_inventory == nullptr) {
            return;
        }

        // get the CatSelector from the StorageItems scene
        CatSelector *p_catselector = nullptr;
        for(auto p_component : *p_storageitems_scene->ComponentLists) {
            MsvcReleaseModeXString type_name = {};
            p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
            if(type_name.as_native_string_view() == "CatSelector") {
                type_name.destroy();
                p_catselector = static_cast<CatSelector *>(p_component);
                break;
            }
            type_name.destroy();
        }
        PROFILER_CHECKPOINT_CHECK(cht, "p_catselector");
        if(p_catselector == nullptr) {
            return;
        }

        // copy all items from game inventory to native map
        std::unordered_map<int64_t, Equipment *> items;
        auto head = p_inventory->storage._List._Myhead;
        auto current = head->_Next;
        while(current != head) {
            // D::debug("item {} {}", current->_Myval.key, current->_Myval.val.name);
            items.try_emplace(current->_Myval.key, &current->_Myval.val);
            current = current->_Next;
        }
        PROFILER_CHECKPOINT_CHECK(cht, "items");

        // get all cats
        std::vector<CatData *> cats;
        CatData *selected_cat = nullptr;
        for(auto p_component : *p_storageitems_scene->ComponentLists) {
            MsvcReleaseModeXString type_name = {};
            p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
            if(type_name.as_native_string_view() != "CatParts") {
                type_name.destroy();
                continue;
            }
            type_name.destroy();
            auto cat = static_cast<CatParts *>(p_component)->cat;
            // D::debug("cat {} {}", cat->sql_key, convert_utf16_wstring_to_utf8_string(cat->name.as_native_wstring_view()));
            cats.push_back(cat);
            if(cat->sql_key == p_catselector->sql_key) {
                selected_cat = cat;
            }
        }
        if(selected_cat == nullptr) {
            D::error("Cannot find selected cat from CatSelector in its scene's CatParts!");
        }
        PROFILER_CHECKPOINT_CHECK(cht, "cats");

        // assort all InventoryItemBoxes
        std::unordered_map<int64_t, InventoryItemBox *> heads;
        std::unordered_map<int64_t, InventoryItemBox *> faces;
        std::unordered_map<int64_t, InventoryItemBox *> necks;
        std::unordered_map<int64_t, InventoryItemBox *> weapons;
        std::unordered_map<int64_t, InventoryItemBox *> trinkets;
        for(auto p_component : *p_storageitems_scene->ComponentLists) {
            MsvcReleaseModeXString type_name = {};
            p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
            if(type_name.as_native_string_view() != "InventoryItemBox") {
                type_name.destroy();
                continue;
            }
            type_name.destroy();
            auto box = static_cast<InventoryItemBox *>(p_component);
            auto kind = box->kind.as_native_string_view();
            std::unordered_map<int64_t, InventoryItemBox *> *map_sel;
            if(kind == "head") {
                map_sel = &heads;
            } else if(kind == "face") {
                map_sel = &faces;
            } else if(kind == "neck") {
                map_sel = &necks;
            } else if(kind == "weapon") {
                map_sel = &weapons;
            } else if(kind == "trinket") {
                map_sel = &trinkets;
            } else {
                D::warn("Unaccounted item category!");
                continue;
            }

            if(auto item = items.find(box->id); item != items.end() && item->second->unknown_3 == 0) {
                // D::debug("assort {} {}", kind, item->second->name);
                (*map_sel)[box->id] = box;
            }
        }
        PROFILER_CHECKPOINT_CHECK(cht, "assort");

        // remove items held by cats from temporary map
        for(auto cat : cats) {
            heads.erase(cat->head.id);
            faces.erase(cat->face.id);
            necks.erase(cat->neck.id);
            weapons.erase(cat->weapon.id);
            trinkets.erase(cat->trinket.id);
        }
        PROFILER_CHECKPOINT_CHECK(cht, "erase");

        auto schedule_randomize = [](std::unordered_map<int64_t, InventoryItemBox *> &map, Equipment &cat_eq) -> bool {
            // D::debug("will randomize {}", cat_eq.name);
            if(cat_eq.id == -1 || cat_eq.unknown_3 == 0) {
                if(map.empty()) {
                    return false;
                }
                auto it = map.begin();
                std::uniform_int_distribution<size_t> dist(0, map.size() - 1);
                std::advance(it, dist(P.prng));
                P.invoke_queue_once_per_update_frame.push_back(&it->second->p_button->stdfunction);
                return true;
            }
            return false;
        };

        // schedule the randomization
        // at most one button can be invoked per frame, otherwise the game will crash
        bool enqueued_action = false;
        enqueued_action |= schedule_randomize(heads, selected_cat->head);
        enqueued_action |= schedule_randomize(faces, selected_cat->face);
        enqueued_action |= schedule_randomize(necks, selected_cat->neck);
        enqueued_action |= schedule_randomize(weapons, selected_cat->weapon);
        enqueued_action |= schedule_randomize(trinkets, selected_cat->trinket);
        if(enqueued_action) {
            // will be unset after P.invoke_queue_once_per_update_frame is drained
            P.block_calls_to_InventoryItemBox__click__lambda_1__Do_call_posttrampoline = true;
        }
        PROFILER_CHECKPOINT_CHECK(cht, "end");
    }
}

// Hook the lambda implementation that we invoke so that a user cannot trigger a crash
// by concurrently triggering randomization and clicking a box
MAKE_HOOK(0, ADDRESS_glaiel__InventoryItemBox__click__lambda_1__Do_call_posttrampoline,
    void, __cdecl, InventoryItemBox__click__lambda_1__Do_call_posttrampoline,
    void *capture
) {
    if(!P.block_calls_to_InventoryItemBox__click__lambda_1__Do_call_posttrampoline) {
        InventoryItemBox__click__lambda_1__Do_call_posttrampoline_hook.orig(capture);
    }
}

// Hook MewDirector's always_update routine to perform tasks in sync with update frames
MAKE_HOOK(0, ADDRESS_glaiel__MewDirector__always_update,
    void, __cdecl, glaiel__MewDirector__always_update,
    MewDirector* thiss
) {
    on_update_frame();
    glaiel__MewDirector__always_update_hook.orig(thiss);
}

// Hook SDL_WaitEventTimeoutNS to capture user input in sync with the game's event processing
// FIXME we really want to hook SDL_PollEvent through GetProcAddress but could not get Mewjector to hook JMP trampolines properly
// SDL_WaitEventTimeoutNS is easier to obtain by direct sig matching
MAKE_HOOK(0, ADDRESS_SDL_WaitEventTimeoutNS,
    bool, __cdecl, SDL_WaitEventTimeoutNS,
    SDL_Event *event, Sint64 timeoutNS
) {
    if(event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_R) {
        MAKE_PROFILER_SCOPE(sct, "SDL_WaitEventTimeoutNS/SDL_EVENT_KEY_DOWN/R");
        shuffle_items_and_schedule_invokes();
    }
    return SDL_WaitEventTimeoutNS_hook.orig(event, timeoutNS);
}
