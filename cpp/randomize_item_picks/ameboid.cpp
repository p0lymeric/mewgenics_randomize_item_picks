#include "ameboid.hpp"
#include "utilities/checksum.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/strings.hpp"
#include "utilities/portal.hpp"

#include <filesystem>

#include <windows.h>

#include "detours.h"

// Ameboid
//
// A base DLL implementation for Mewgenics modding.
//
// polymeric 2026

GlobalContext G;

enum class AmeboidErrorCode {
    Success,
    HashMismatch,
    FailedToHook,
    FailedToUnhook,
};

std::string get_user_facing_error_message(AmeboidErrorCode error_code) {
    std::string builder;
    switch(error_code) {
        case AmeboidErrorCode::Success:
            builder += "A supposedly impossible error occurred where something failed successfully.\n";
            builder += "(but seriously, the author of this mod probably did something very silly if you are seeing this message)\n";
            break;
        case AmeboidErrorCode::HashMismatch:
            builder += std::format("The version of {} installed is not compatible with the version of Mewgenics on your computer.\n", MOD_NAME);
            break;
        case AmeboidErrorCode::FailedToHook:
            builder += std::format("A function hook failed to install.\n", MOD_NAME);
            break;
        case AmeboidErrorCode::FailedToUnhook:
            builder += std::format("A function hook failed to uninstall.\n", MOD_NAME);
            break;
    }

    builder += std::format("\nPlease check for an update or report an issue at:\n{}\n\n", MOD_URL);

    builder += std::format("Additional information:\n");
    builder += std::format("Mod: {} version {}\n", MOD_NAME, MOD_VERSION);
    builder += std::format("Mewgenics.exe expected SHA-256: {}\n", hash256bit_to_string(EXE_SHA256));
    builder += std::format("Mewgenics.exe actual SHA-256: {}", G.exe_actual_sha256.has_value() ? hash256bit_to_string(G.exe_actual_sha256.value()) : "<unknown>");

    return builder;
}

AmeboidErrorCode on_attach() {
    // Actual virtual address where mapped executable begins
    uintptr_t host_exec_base_va = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    G.host_exec_base_va = host_exec_base_va;

    // Calculate the SHA-256 digest of the executable
    // If the act of calculating the hash fails, continue optimistically
    std::filesystem::path exe_path = get_module_file_path(NULL);
    G.exe_actual_sha256 = sha256_file(exe_path);
    if(G.exe_actual_sha256.has_value()) {
        G.exe_hash_mismatch_detected = (G.exe_actual_sha256.value() != EXE_SHA256);
    }

    // Create a Win32 console window with which to print log messages. ENABLE_CONSOLE_LOGGING disables this for public release.
    ALLOC_CONSOLE();

    D::info("Initializing {}", MOD_NAME);
    // D::info("Hook base VA: 0x{:x}", G.dll_base_va);
    // D::info("Executable base VA: 0x{:x}", host_exec_base_va);
    // D::info("Executable SHA-256: {}", G.exe_actual_sha256.has_value() ? hash256bit_to_string(G.exe_actual_sha256.value()) : "<unknown>");

    // Do not install any hooks if a hash mismatch was detected.
    // Instead exit this function. The DLL will be loaded, but it will effectively be inactive.
    if(G.exe_hash_mismatch_detected) {
        return AmeboidErrorCode::HashMismatch;
    }

    // Resolve portals (trampolines to functions and data)
    SPortalRegistry::resolve_portals(host_exec_base_va);

    // Try to install function hooks
    if(SFunctionHookRegistry::api_is_present(EFunctionHookProvider::Mewjector)) {
        // Use Mewjector if present for coordinated hooking
        G.dll_can_self_eject = false;
        if(!SFunctionHookRegistry::install_hooks(host_exec_base_va, EFunctionHookProvider::Mewjector, 0)) {
            return AmeboidErrorCode::FailedToHook;
        }
    } else {
        G.dll_can_self_eject = true;
        if(!SFunctionHookRegistry::install_hooks(host_exec_base_va, EFunctionHookProvider::Detours, 0)) {
            return AmeboidErrorCode::FailedToHook;
        }
    }

    return AmeboidErrorCode::Success;
}

AmeboidErrorCode on_unload_detach() {
    D::info("Uninitializing {} (Unload)", MOD_NAME);
    // Try to gracefully remove our hooks if this dll was ejected by an external tool (e.g. via System Informer).
    if(!SFunctionHookRegistry::uninstall_hooks_all(true)) {
        return AmeboidErrorCode::FailedToUnhook;
    }
    return AmeboidErrorCode::Success;
}

AmeboidErrorCode on_exitprocess_detach() {
    D::info("Uninitializing {} (ExitProcess)", MOD_NAME);
    // May as well clean up resources properly, even if the process is going down.
    // Remove hooks, but don't fail if the API is unable to uninstall hooks.
    if(!SFunctionHookRegistry::uninstall_hooks_all(false)) {
        return AmeboidErrorCode::FailedToUnhook;
    }
    return AmeboidErrorCode::Success;
}

void on_error(bool is_detach, AmeboidErrorCode error_code) {
    std::string user_facing_error_message = get_user_facing_error_message(error_code);
    if(is_detach) {
        D::error("An unrecoverable error occurred during uninitialization.");
        D::error("{}", user_facing_error_message);
        // don't pop up an error message on uninit
    } else {
        D::error("An unrecoverable error occurred during initialization.");
        D::error("{}", user_facing_error_message);

        // would be good to flash a user-visible message, but an exclusive fullscreen window will aggressively fight to be on top and obscure the error message
        // std::wstring caption_wstring = std::format(L"Error - {}", convert_utf8_string_to_utf16_wstring(MOD_NAME));
        // std::wstring user_facing_error_message_wstring = convert_utf8_string_to_utf16_wstring(user_facing_error_message);
        // MessageBoxW(NULL, user_facing_error_message_wstring.c_str(), caption_wstring.c_str(), MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
}

void final_rites() {
    FREE_CONSOLE();
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved    // reserved
) {
    if(DetourIsHelperProcess()) {
        return TRUE;
    }

    AmeboidErrorCode error_code = AmeboidErrorCode::Success;

    // Perform actions based on the reason for calling.
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            DetourRestoreAfterWith();

            G.dll_base_va = reinterpret_cast<uintptr_t>(hinstDLL);

            error_code = on_attach();

            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            // Perform any necessary cleanup.
            if(lpReserved == NULL) {
                // traversed when the dll is ejected
                error_code = on_unload_detach();
            } else {
                // traversed when Mewgenics is exiting
                error_code = on_exitprocess_detach();
            }
            break;
    }

    if(error_code != AmeboidErrorCode::Success) {
        on_error(fdwReason == DLL_PROCESS_DETACH, error_code);
    }

    if(fdwReason == DLL_PROCESS_DETACH) {
        final_rites();
    }

    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
