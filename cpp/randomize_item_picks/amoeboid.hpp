#pragma once

#include "utilities/checksum.hpp"
#include "utilities/signature.hpp"

#include <cstdint>
#include <optional>

// Main program declarations.
//
// polymeric 2026

// CONSTANTS

// Mod information

inline constexpr char MOD_AUTHOR[] = "polymeric";
inline constexpr char MOD_NAME[] = "Randomize Item Picks";
inline constexpr char MOD_URL[] = "https://www.nexusmods.com/mewgenics/mods/318\nhttps://github.com/p0lymeric/mewgenics_randomize_item_picks";
inline constexpr char MOD_VERSION[] = "1.0.2";

// These addresses were extracted from Mewgenics.exe
// The script under misc/find_rvas.py can help with recovering these addresses after a game update

// Semantic release version of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr char EXE_VERSION[] = "1.0.20941";

// SHA-256 hash of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("c10cb2435874db1e291b949eb226e061512e05f2bc235504a6617f525688b26c");

// Function offsets are encoded as relative VAs
inline constexpr const auto ADDRESS_glaiel__MewDirector__always_update = DirectSig::make<"48 8B 05 ?? ?? ?? ?? F2 0F 10 05 ?? ?? ?? ?? 48 FF 81 30 05 00 00 F2 0F 5E 80 C8 0D 00 00 F2 0F 58 81 38 05 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__InventoryItemBox__click__lambda_1__Do_call_posttrampoline = DirectSig::make<"40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 E1 48 81 EC E8 00 00 00 4C 8B E9 C7 45 67 00 00 00 00">(0);
inline constexpr const auto ADDRESS_SDL_WaitEventTimeoutNS = DirectSig::make<"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 81 EC B0 00 00 00 48 85 D2">(0);

// Data offsets are encoded as relative VAs
inline constexpr const auto DATAOFF_glaiel__MewDirector__p_singleton = IndirectSig::make<"48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00">(21, 4, true, true);

// TLS variable offsets are encoded relative to the base VA of their TLS slot

// CROSS-TU DECLARATIONS

// The "everything" struct
// Exporter: amoeboid.cpp
struct GlobalContext;
extern GlobalContext G;

// TYPE DECLARATIONS

struct GlobalContext {
    // amoeboid.dll offset.
    uintptr_t dll_base_va;
    uintptr_t dll_image_size;

    // Mewgenics.exe offset.
    uintptr_t host_exec_base_va;
    uintptr_t host_exec_image_size;

    // Whether it is permissible for the dll to self-eject.
    // (false if the dll cannot self-uninstall its hooks)
    bool dll_can_self_eject;

    // Mewgenics.exe hash.
    std::optional<Hash256Bit> exe_actual_sha256;
    bool exe_hash_mismatch_detected;
};
