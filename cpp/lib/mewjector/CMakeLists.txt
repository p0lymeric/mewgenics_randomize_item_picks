cmake_minimum_required(VERSION 3.15)

project(mewjector)

# Set language mode
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC
set(CMAKE_C_EXTENSIONS OFF)

set(MEWJECTOR_DIR third_party)

if(MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(/W4)
    else()
        add_compile_options(/W4)
    endif()
elseif(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define build targets
add_library(mewjector INTERFACE)

target_include_directories(mewjector INTERFACE
    ${MEWJECTOR_DIR}
)
