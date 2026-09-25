cmake_minimum_required(VERSION 3.16)
project(veebhaos VERSION 1.0.0 LANGUAGES C)

# Strict C11 Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Developer & Debugging Flags
option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_compile_options(-Wall -Wextra -pedantic -g)
add_definitions(-DCONFIG_SIMULATOR=1)

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
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator/lv_conf.h" CACHE STRING "Path to lv_conf.h")
set(LV_CONF_INCLUDE_SIMPLE ON CACHE BOOL "Include lv_conf.h simply")
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "Disable LVGL examples")
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "Disable LVGL demos")
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "Disable ThorVG")

add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)

# Board & FreeRTOS Sources
set(SIM_SOURCES
    boards/simulator/sim_main.c
    boards/simulator/sim_keyboard_map.c
    boards/simulator/board_info.c
    boards/simulator/drivers/hal_display_sim.c
    boards/simulator/drivers/hal_keypad_sim.c
    boards/simulator/drivers/hal_power_sim.c
    boards/simulator/drivers/hal_audio_sim.c
    boards/simulator/port/port.c
    boards/simulator/port/utils/wait_for_event.c
    boards/simulator/port/heap_3.c
    kernel/freertos/Source/tasks.c
    kernel/freertos/Source/queue.c
    kernel/freertos/Source/list.c
    kernel/freertos/Source/timers.c
    kernel/freertos/Source/event_groups.c
    kernel/freertos/Source/stream_buffer.c
    sdk/core/os_kernel.c
    sdk/core/softkey_bar.c
    sdk/core/status_bar.c
    sdk/core/win_mgr.c
    sdk/core/theme_mgr.c
    sdk/core/wallpaper.c
    sdk/core/live_pill.c
    sdk/core/veebha_log.c
    sdk/core/os_event.c
    drivers/irq/irq_manager.c
    sdk/input/t9_engine.c
    sdk/templates/tpl_list.c
    sdk/templates/tpl_grid.c
    sdk/templates/tpl_dialog.c
    sdk/templates/tpl_editor.c
    sdk/templates/tpl_media.c
    sdk/core/app_registry.c
    apps/overlays/notif_panel.c
    apps/overlays/task_mgr.c
    apps/common/mock_telephony.c
    apps/common/lazy_load.c
    apps/screens/template_screen.c
    apps/home/app_idle.c
    apps/home/app_launcher.c
    apps/dialer/app_dialer.c
    apps/telephony/app_incall.c
    apps/contacts/app_contacts.c
    apps/messages/app_messages.c
    apps/calllogs/app_calllogs.c
    apps/music/app_music.c
    apps/files/app_files.c
    apps/settings/app_settings.c
    apps/settings/app_bt.c
    apps/settings/app_bt_scan.c
    apps/settings/app_tethering.c
    apps/calendar/app_calendar.c
    apps/tools/app_tools.c
    apps/tools/app_calc.c
    apps/tools/app_stopwatch.c
    apps/tools/app_alarm.c
    apps/tools/app_textread.c
    apps/tools/app_funzone.c
    apps/browser/app_browser.c
    apps/game/app_game.c
    apps/gallery/app_gallery.c
    apps/recorder/app_recorder.c
    apps/overlays/boot_screen.c
    apps/overlays/screen_saver.c
    apps/overlays/usb_select.c
    apps/vapps/vapp_tetris.c
    apps/vapps/vapp_brick_breaker.c
    apps/vapps/vapp_unit_converter.c
    apps/vapps/vapp_morse_flasher.c
    apps/vapps/vapp_chip_synth.c
    apps/vapps/vapp_sys_monitor.c
    drivers/mock/mock_connectivity.c
    drivers/mock/mock_hardware.c
    sdk/vfs/os_vfs.c
    sdk/core/vapp_loader.c
    sdk/storage/os_nvram.c
    fonts/veebha_font_latin_cyrillic_14.c
    fonts/veebha_font_devanagari_16.c
    sdk/text/indic_shaper.c
    sdk/text/font_fallback.c
    sdk/core/veebha_i18n.c
    sdk/i18n/strings_en.c
    sdk/i18n/strings_hi.c
    sdk/i18n/strings_ru.c
)

# Main executable target: veebha_os
add_executable(veebha_os ${SIM_SOURCES})

target_include_directories(veebha_os PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/boards
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator/port
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/simulator/port/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/core
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/vfs
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/text
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/telephony
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/files
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/calendar
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/game
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/gallery
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/recorder
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/overlays
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/vapps
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvgl
)

# Link SDL2
if(TARGET SDL2::SDL2)
    target_link_libraries(veebha_os PRIVATE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(veebha_os PRIVATE SDL2::SDL2-static)
elseif(SDL2_LIBRARIES)
    target_link_libraries(veebha_os PRIVATE ${SDL2_LIBRARIES})
    target_include_directories(veebha_os PRIVATE ${SDL2_INCLUDE_DIRS})
else()
    target_link_libraries(veebha_os PRIVATE SDL2)
endif()

# Link LVGL and standard POSIX system libraries
target_link_libraries(veebha_os PRIVATE lvgl m pthread)

# Also create veebhaos_sim symlink/alias so both binary names work
add_custom_target(veebhaos_sim ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_NAME:veebha_os> ${CMAKE_CURRENT_BINARY_DIR}/veebhaos_sim
    DEPENDS veebha_os
)
