####################################################################
# Automatically-generated file. Do not edit!                       #
####################################################################

set(SDK_PATH "/Users/miyablon/.silabs/slt/installs/conan/p/simpleb526998f4a4d/p")
set(COPIED_SDK_PATH "simplicity_sdk_2025.6.2")
set(PKG_PATH "/Users/miyablon/.silabs/slt/installs")

add_library(slc OBJECT
    "${SDK_PATH}/platform/common/src/sl_assert.c"
    "${SDK_PATH}/platform/common/src/sl_core_cortexm.c"
    "${SDK_PATH}/platform/common/src/sl_syscalls.c"
    "${SDK_PATH}/platform/Device/SiliconLabs/EFR32MG24/Source/startup_efr32mg24.c"
    "${SDK_PATH}/platform/Device/SiliconLabs/EFR32MG24/Source/system_efr32mg24.c"
    "${SDK_PATH}/platform/driver/gpio/src/sl_gpio.c"
    "${SDK_PATH}/platform/emlib/src/em_cmu.c"
    "${SDK_PATH}/platform/emlib/src/em_emu.c"
    "${SDK_PATH}/platform/emlib/src/em_eusart.c"
    "${SDK_PATH}/platform/emlib/src/em_gpio.c"
    "${SDK_PATH}/platform/emlib/src/em_ldma.c"
    "${SDK_PATH}/platform/emlib/src/em_msc.c"
    "${SDK_PATH}/platform/emlib/src/em_prs.c"
    "${SDK_PATH}/platform/emlib/src/em_system.c"
    "${SDK_PATH}/platform/emlib/src/em_timer.c"
    "${SDK_PATH}/platform/emlib/src/em_usart.c"
    "${SDK_PATH}/platform/emlib/src/em_wdog.c"
    "${SDK_PATH}/platform/peripheral/src/sl_hal_gpio.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/pa-conversions/pa_curves_efr32.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ble_config_38M4Hz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ble_config_39MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ble_config_40MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ieee802154_config_38M4Hz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ieee802154_config_39MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_ieee802154_config_40MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_rfsense_ook_config_38M4Hz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_rfsense_ook_config_39MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24/sl_rail_rfsense_ook_config_40MHz.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/sl_rail_phy_overrides.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_sequencer/sl_rail_util_sequencer.c"
    "${SDK_PATH}/platform/service/clock_manager/src/sl_clock_manager.c"
    "${SDK_PATH}/platform/service/clock_manager/src/sl_clock_manager_hal_s2.c"
    "${SDK_PATH}/platform/service/clock_manager/src/sl_clock_manager_init.c"
    "${SDK_PATH}/platform/service/clock_manager/src/sl_clock_manager_init_hal_s2.c"
    "${SDK_PATH}/platform/service/device_init/src/sl_device_init_dcdc_s2.c"
    "${SDK_PATH}/platform/service/device_init/src/sl_device_init_emu_s2.c"
    "${SDK_PATH}/platform/service/device_manager/clocks/sl_device_clock_efr32xg24.c"
    "${SDK_PATH}/platform/service/device_manager/src/sl_device_clock.c"
    "${SDK_PATH}/platform/service/device_manager/src/sl_device_gpio.c"
    "${SDK_PATH}/platform/service/interrupt_manager/src/sl_interrupt_manager_cortexm.c"
    "${SDK_PATH}/platform/service/memory_manager/src/sl_memory_manager_region.c"
    "${SDK_PATH}/platform/service/mpu/src/sl_mpu_s2.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init_memory.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_process_action.c"
    "../app_init.c"
    "../app_process.c"
    "../autogen/rail_config.c"
    "../autogen/sl_event_handler.c"
    "../autogen/sl_rail_util_callbacks.c"
    "../autogen/sl_rail_util_init.c"
    "../main.c"
)

target_include_directories(slc PUBLIC
   "../autogen"
   "../config"
   "../config/rail"
   "../."
    "${SDK_PATH}/platform/Device/SiliconLabs/EFR32MG24/Include"
    "${SDK_PATH}/platform/service/clock_manager/inc"
    "${SDK_PATH}/platform/service/clock_manager/src"
    "${SDK_PATH}/platform/CMSIS/Core/Include"
    "${SDK_PATH}/platform/common/inc"
    "${SDK_PATH}/platform/service/device_manager/inc"
    "${SDK_PATH}/platform/service/device_init/inc"
    "${SDK_PATH}/platform/emlib/inc"
    "${SDK_PATH}/platform/driver/gpio/inc"
    "${SDK_PATH}/platform/peripheral/inc"
    "${SDK_PATH}/platform/service/interrupt_manager/inc"
    "${SDK_PATH}/platform/service/interrupt_manager/src"
    "${SDK_PATH}/platform/service/interrupt_manager/inc/arm"
    "${SDK_PATH}/platform/service/memory_manager/inc"
    "${SDK_PATH}/platform/service/mpu/inc"
    "${SDK_PATH}/platform/radio/rail_lib/common"
    "${SDK_PATH}/platform/radio/rail_lib/protocol/ble"
    "${SDK_PATH}/platform/radio/rail_lib/protocol/ieee802154"
    "${SDK_PATH}/platform/radio/rail_lib/protocol/wmbus"
    "${SDK_PATH}/platform/radio/rail_lib/protocol/zwave"
    "${SDK_PATH}/platform/radio/rail_lib/chip/efr32/efr32xg2x"
    "${SDK_PATH}/platform/radio/rail_lib/protocol/sidewalk"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_built_in_phys/efr32xg24"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_callbacks"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/pa-conversions"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/pa-conversions/efr32xg24"
    "${SDK_PATH}/platform/radio/rail_lib/plugin/rail_util_protocol"
    "${SDK_PATH}/platform/service/sl_main/inc"
    "${SDK_PATH}/platform/service/sl_main/src"
)

target_compile_definitions(slc PUBLIC
    "EFR32MG24B220F1536IM48=1"
    "SL_CODE_COMPONENT_SYSTEM=system"
    "SL_CLOCK_MANAGER_AUTO_BAND_VALID=1"
    "SL_CODE_COMPONENT_CLOCK_MANAGER=clock_manager"
    "SL_COMPONENT_CATALOG_PRESENT=1"
    "SL_CODE_COMPONENT_GPIO=gpio"
    "SL_CODE_COMPONENT_HAL_COMMON=hal_common"
    "SL_CODE_COMPONENT_HAL_GPIO=hal_gpio"
    "SL_CODE_COMPONENT_INTERRUPT_MANAGER=interrupt_manager"
    "CMSIS_NVIC_VIRTUAL=1"
    "CMSIS_NVIC_VIRTUAL_HEADER_FILE=\"cmsis_nvic_virtual.h\""
    "SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0"
    "SL_RAIL_UTIL_PA_CONFIG_HEADER=<sl_rail_util_pa_config.h>"
    "SL_CODE_COMPONENT_CORE=core"
)

target_link_libraries(slc PUBLIC
    "-Wl,--start-group"
    "gcc"
    "c"
    "m"
    "nosys"
    "${SDK_PATH}/platform/radio/rail_lib/autogen/librail_release/librail_efr32xg24_gcc_release.a"
    "-Wl,--end-group"
)
target_compile_options(slc PUBLIC
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:C>:-mthumb>
    $<$<COMPILE_LANGUAGE:C>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:C>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:C>:-mcmse>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Og>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-g>
    $<$<COMPILE_LANGUAGE:C>:-fno-lto>
    $<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:CXX>:-mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:CXX>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-mcmse>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Og>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:CXX>:-g>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-lto>
    $<$<COMPILE_LANGUAGE:CXX>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:ASM>:-mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>
    "$<$<COMPILE_LANGUAGE:ASM>:SHELL:-x assembler-with-cpp>"
)

set(post_build_command )
set_property(TARGET slc PROPERTY C_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_EXTENSIONS OFF)

target_link_options(slc INTERFACE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    "-T${CMAKE_CURRENT_LIST_DIR}/../autogen/linkerfile.ld"
    --specs=nano.specs
    -Wl,-Map=$<TARGET_FILE_DIR:TX_Board>/TX_Board.map
    -fno-lto
    -Wl,--gc-sections
)

# BEGIN_SIMPLICITY_STUDIO_METADATA=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END_SIMPLICITY_STUDIO_METADATA