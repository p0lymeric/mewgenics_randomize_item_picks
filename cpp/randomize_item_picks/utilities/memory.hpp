#pragma once

#include <windows.h>

// Virtual memory utilities
//
// polymeric 2026

// Gets the base pointer for thread-local-storage slot 0.
template<typename T>
T *get_tls0_base() {
    return reinterpret_cast<T **>(__readgsqword(0x58))[0];
}

// Allocate onto the host process' heap
inline void *host_alloc(size_t size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

// Free from the host process' heap
inline void host_free(void *ptr) {
    HeapFree(GetProcessHeap(), 0, ptr);
}

// Reallocate within the host process' heap
inline void *host_realloc(void *ptr, size_t size) {
    // based off MSVC's internal realloc (which conditionally dispatches to HeapAlloc or HeapReAlloc)
    if(ptr == nullptr) {
        return HeapAlloc(GetProcessHeap(), 0, size);
    } else {
        return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
    }
}
