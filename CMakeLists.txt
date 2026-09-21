cmake_minimum_required(VERSION 3.16)
project(veebhaos VERSION 0.1.0 LANGUAGES C)

# Strict C11 Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Developer & Debugging Flags
option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_compile_options(-Wall -Wextra -pedantic -g)

if(ENABLE_ASAN)
    message(STATUS "Sanitizers enabled (-fsanitize=address,undefined)")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Locate SDL2 Dependency
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# LVGL v9 Configuration Integration
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_INCLUDE_SIMPLE ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)

# Simulator & Core SDK Sources
set(SIM_SOURCES
    boards/simulator/sim_main.c
    boards/simulator/sim_keyboard_map.c
    drivers/sim/sdl_display.c
    drivers/sim/sdl_input.c
    sdk/core/softkey_bar.c
    sdk/core/win_mgr.c
    sdk/templates/tpl_list.c
    sdk/templates/tpl_grid.c
)

# Simulator Executable Target
add_executable(veebhaos_sim ${SIM_SOURCES})

target_include_directories(veebhaos_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/boards
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvgl
)

# Link SDL2
if(TARGET SDL2::SDL2)
    target_link_libraries(veebhaos_sim PRIVATE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(veebhaos_sim PRIVATE SDL2::SDL2-static)
elseif(SDL2_LIBRARIES)
    target_link_libraries(veebhaos_sim PRIVATE ${SDL2_LIBRARIES})
    target_include_directories(veebhaos_sim PRIVATE ${SDL2_INCLUDE_DIRS})
else()
    target_link_libraries(veebhaos_sim PRIVATE SDL2)
endif()

# Link LVGL and standard POSIX system libraries
target_link_libraries(veebhaos_sim PRIVATE lvgl m pthread)
