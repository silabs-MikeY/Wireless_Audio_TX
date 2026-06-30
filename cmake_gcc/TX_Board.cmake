####################################################################
# Automatically-generated file. Do not edit!                       #
####################################################################

set(SDK_PATH "/Users/miyablon/.silabs/slt/installs/conan/p/simpleb526998f4a4d/p")
set(COPIED_SDK_PATH "simplicity_sdk_2025.6.2")
set(PKG_PATH "/Users/miyablon/.silabs/slt/installs")

add_library(slc OBJECT
    "${SDK_PATH}/platform/common/src/sl_assert.c"
    "${SDK_PATH}/platform/common/src/sl_core_cortexm.c"
    "${SDK_PATH}/platform/common/src/sl_slist.c"
    "${SDK_PATH}/platform/common/src/sl_string.c"
    "${SDK_PATH}/platform/common/src/sl_syscalls.c"
    "${SDK_PATH}/platform/Device/SiliconLabs/EFR32MG24/Source/startup_efr32mg24.c"
    "${SDK_PATH}/platform/Device/SiliconLabs/EFR32MG24/Source/system_efr32mg24.c"
    "${SDK_PATH}/platform/driver/gpio/src/sl_gpio.c"
    "${SDK_PATH}/platform/emdrv/dmadrv/src/dmadrv.c"
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
    "${SDK_PATH}/platform/peripheral/src/sl_hal_eusart.c"
    "${SDK_PATH}/platform/peripheral/src/sl_hal_gpio.c"
    "${SDK_PATH}/platform/peripheral/src/sl_hal_prs.c"
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
    "${SDK_PATH}/platform/service/device_manager/devices/sl_device_peripheral_hal_efr32xg24.c"
    "${SDK_PATH}/platform/service/device_manager/src/sl_device_clock.c"
    "${SDK_PATH}/platform/service/device_manager/src/sl_device_gpio.c"
    "${SDK_PATH}/platform/service/device_manager/src/sl_device_peripheral.c"
    "${SDK_PATH}/platform/service/interrupt_manager/src/sl_interrupt_manager_cortexm.c"
    "${SDK_PATH}/platform/service/iostream/src/sl_iostream.c"
    "${SDK_PATH}/platform/service/iostream/src/sl_iostream_eusart.c"
    "${SDK_PATH}/platform/service/iostream/src/sl_iostream_retarget_stdio.c"
    "${SDK_PATH}/platform/service/iostream/src/sl_iostream_uart.c"
    "${SDK_PATH}/platform/service/memory_manager/src/sl_memory_manager_region.c"
    "${SDK_PATH}/platform/service/mpu/src/sl_mpu_s2.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init_memory.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_process_action.c"
    "${SDK_PATH}/util/third_party/printf/printf.c"
    "${SDK_PATH}/util/third_party/printf/src/iostream_printf.c"
    "../app_init.c"
    "../app_process.c"
    "../autogen/rail_config.c"
    "../autogen/sl_event_handler.c"
    "../autogen/sl_iostream_handles.c"
    "../autogen/sl_iostream_init_eusart_instances.c"
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
    "${SDK_PATH}/platform/common/inc"
    "${SDK_PATH}/platform/service/clock_manager/inc"
    "${SDK_PATH}/platform/service/clock_manager/src"
    "${SDK_PATH}/platform/CMSIS/Core/Include"
    "${SDK_PATH}/platform/service/device_manager/inc"
    "${SDK_PATH}/platform/service/device_init/inc"
    "${SDK_PATH}/platform/emdrv/dmadrv/inc"
    "${SDK_PATH}/platform/emdrv/dmadrv/inc/s2_signals"
    "${SDK_PATH}/platform/emdrv/common/inc"
    "${SDK_PATH}/platform/emlib/inc"
    "${SDK_PATH}/platform/driver/gpio/inc"
    "${SDK_PATH}/platform/peripheral/inc"
    "${SDK_PATH}/platform/service/interrupt_manager/inc"
    "${SDK_PATH}/platform/service/interrupt_manager/src"
    "${SDK_PATH}/platform/service/interrupt_manager/inc/arm"
    "${SDK_PATH}/platform/service/iostream/inc"
    "${SDK_PATH}/platform/service/memory_manager/inc"
    "${SDK_PATH}/platform/service/mpu/inc"
    "${SDK_PATH}/util/third_party/printf"
    "${SDK_PATH}/util/third_party/printf/inc"
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
    "SL_CODE_COMPONENT_DEVICE_PERIPHERAL=device_peripheral"
    "SL_CODE_COMPONENT_DMADRV=dmadrv"
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
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin-printf>
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin-sprintf>
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
    $<$<COMPILE_LANGUAGE:CXX>:-fno-builtin-printf>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-builtin-sprintf>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-lto>
    $<$<COMPILE_LANGUAGE:CXX>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:ASM>:-mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>
    "$<$<COMPILE_LANGUAGE:ASM>:SHELL:-x assembler-with-cpp>"
)

set(COMMANDER_DEVICE "EFR32MG24B220F1536IM48")
set(COMMANDER_SERIALNO "440305682")

set(post_build_command
    "${POST_BUILD_EXE}"
    "flash"
    "$<TARGET_FILE_DIR:TX_Board>/TX_Board.hex"
    "--device"
    "${COMMANDER_DEVICE}"
    "--serialno"
    "${COMMANDER_SERIALNO}"
    "--halt"
)
set_property(TARGET slc PROPERTY C_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_EXTENSIONS OFF)

target_link_options(slc INTERFACE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -T${CMAKE_CURRENT_LIST_DIR}/../autogen/linkerfile.ld
    --specs=nano.specs
    "SHELL:-Xlinker -Map=$<TARGET_FILE_DIR:TX_Board>/TX_Board.map"
    -fno-lto
    -Wl,--gc-sections
)

# BEGIN_SIMPLICITY_STUDIO_METADATA=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=END_SIMPLICITY_STUDIO_METADATA