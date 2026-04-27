cmake_minimum_required(VERSION 3.15)

project(ameboid)

# Read third-party library CMakeLists
add_subdirectory(lib/Detours EXCLUDE_FROM_ALL)
add_subdirectory(lib/libtomcrypt EXCLUDE_FROM_ALL)
add_subdirectory(lib/mewjector EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Read our CMakeLists
add_subdirectory(randomize_item_picks)
# add_subdirectory(cosmic_ooze)
