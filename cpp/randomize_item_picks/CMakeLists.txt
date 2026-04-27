cmake_minimum_required(VERSION 3.15)

project(randomize_item_picks)

# Build target names
set(HOOK_NAME randomize_item_picks)

# Set language mode (C++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC; permissive- is set by std:c++20
set(CMAKE_CXX_EXTENSIONS OFF)

# Set language mode (C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
elseif(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define build targets
add_library(${HOOK_NAME} SHARED
    ameboid.cpp
    ameboid.def
    randomize_item_picks.cpp
    utilities/mewjector_support.c
)

target_include_directories(${HOOK_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${HOOK_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Statically link libc++ (static-libstdc++ flag also covers libc++)
        target_link_options(${HOOK_NAME} PRIVATE -static-libstdc++)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Statically link libgcc/libstdc++/libwinpthread
        target_link_options(${HOOK_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
    endif()
    # Don't prefix the hook dll with "lib" in its filename
    set_target_properties(${HOOK_NAME} PROPERTIES PREFIX "")
endif()

target_link_libraries(${HOOK_NAME} PRIVATE
    Detours
    libtomcrypt
    mewjector
    SDL3::Headers
)

install(TARGETS ${TARGET_NAME} ${HOOK_NAME} RUNTIME DESTINATION .)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${HOOK_NAME}> DESTINATION . OPTIONAL)
endif()
