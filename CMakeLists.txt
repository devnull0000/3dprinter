cmake_minimum_required (VERSION 3.7.0)
project (mcu-soft C)
enable_language(ASM)

add_definitions(-DSTM32F765xx -DUSE_USB_FS) # -DMISSING_SYSCALL_NAMES -DREENTRANT_SYSCALLS_PROVIDED)
include_directories (
src 
CMSIS/Include
CMSIS/Device/ST/STM32F7xx/Include
STM32F7xx_HAL_Driver/Inc
STM32_USB_Device_Library/Core/Inc
STM32_USB_Device_Library/Class/CDC/Inc
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m7 -mthumb -O0 -ggdb -std=c99")

#--specs=nosys.specs to compiler
set (SRC_FILES 
startup.s 
newlib-supp/newlib-supp.c
linker_script.c
src/main.c
src/command.c
src/motor.c
src/stm32f7xx_it.c
src/system_stm32f7xx.c
src/usbd_cdc_interface.c
src/usbd_conf.c
src/usbd_desc.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c
STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c
STM32_USB_Device_Library/Core/Src/usbd_core.c
STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
)

#SET_SOURCE_FILES_PROPERTIES(linker_script.c PROPERTIES OBJECT_DEPENDS STM32F765ZGT6_FLASH.ld)
set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/STM32F765ZGT6_FLASH.ld")

add_executable (${PROJECT_NAME}.elf ${SRC_FILES}) 
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/STM32F765ZGT6_FLASH.ld)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
                   COMMAND ${CMAKE_OBJCOPY} -Obinary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
                   COMMENT making ${PROJECT_NAME}.bin from ${PROJECT_NAME}.elf)
